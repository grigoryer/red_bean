#include "uci.hpp"
#include "debug.hpp"
#include "global.hpp"
#include "move.hpp"
#include <iostream>
#include <future>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include "move_making.hpp"

namespace 
{
    constexpr int64_t kNoTimeLimit = std::numeric_limits<int64_t>::max() / 4;
    
    inline Square StringToSquare(const std::string& sqr_str) 
    {
        if (sqr_str.length() != 2) return Square::kNoSquare;
        
        Column col = static_cast<Column>(sqr_str[0] - 'a');
        Row row = static_cast<Row>(sqr_str[1] - '1');
        
        if (row > Row::_8 || col > Column::H) { return Square::kNoSquare; }
        
        return static_cast<Square>((static_cast<int>(row) * 8) + static_cast<int>(col));
    }
    
    inline ExtMove StringToMove(const std::string& move_string, Board& board)
    {
        if (move_string.length() < 4) { return ExtMove{}; }
    
        Square from_sqr = StringToSquare(move_string.substr(0, 2));
        Square to_sqr = StringToSquare(move_string.substr(2, 2));
        if (from_sqr == Square::kNoSquare || to_sqr == Square::kNoSquare) { return ExtMove{}; }
    
        Piece piece = board.GetPieceOnSquare(from_sqr);
    
        if (move_string.length() >= 5)
        {
            char promo_char = move_string[4];
            switch (promo_char)
            {
                case 'q': return ExtMove{Move{from_sqr, to_sqr, MoveType::kPromotion, Piece::kQueen}};
                case 'r': return ExtMove{Move{from_sqr, to_sqr, MoveType::kPromotion, Piece::kRook}};
                case 'b': return ExtMove{Move{from_sqr, to_sqr, MoveType::kPromotion, Piece::kBishop}};
                case 'n': return ExtMove{Move{from_sqr, to_sqr, MoveType::kPromotion, Piece::kKnight}};
                default: return ExtMove{};
            }
        }
    
        if (piece == Piece::kPawn)
        {
            if (to_sqr == board.GetCurStatus().enpassant_)
            {
                return ExtMove{Move{from_sqr, to_sqr, MoveType::kEnPassant}};
            }
        }
        else if (piece == Piece::kKing)
        {
            int file_diff = std::abs(static_cast<int>(GetSquareCol(from_sqr)) - static_cast<int>(GetSquareCol(to_sqr)));
            if (file_diff == 2)
            {
                return ExtMove{Move{from_sqr, to_sqr, MoveType::kCastle}};
            }
        }
    
        return ExtMove{Move{from_sqr, to_sqr, MoveType::kNormal}};
    }

}  // namespace


void SearcherThread::IdleLoop(ThreadPool& pool) 
{
    std::unique_lock lck(pool.mtx_, std::defer_lock);
    while (true) 
    {
        lck.lock();
        pool.cv_.wait(lck, [&]{ return searching_ || pool.quit_flag_; });
        
        if (pool.quit_flag_) { return; }

        lck.unlock();

        best_move = searcher_.FindBestMove(board_, search_args.soft_ms, search_args.depth, search_args.start_depth ,id_);   // polls pool.stop_flag_ inside
        
        lck.lock();
        searching_ = false;
        lck.unlock();
        pool.cv_.notify_all();
    }
}

void UCI::SetPosition(const std::string& input)
{
    std::istringstream stream(input);
    std::string token;

    if (!(stream >> token)) { return; }

    if (token == "startpos")
    {
        searcher_threads_.pool_[0]->board_.ResetBoard();
        token.clear();
        stream >> token;  // may be "moves" or nothing
    }
    else if (token == "fen")
    {
        std::string fen;
        while (stream >> token && token != "moves")
        {
            if (!fen.empty()) { fen += ' '; }
            fen += token;
        }
        if (fen.empty()) { return; }
        searcher_threads_.pool_[0]->board_.ResetBoard(fen);
    }
    else
    {
        return;
    }

    if (token != "moves") { return; }

    while (stream >> token)
    {
        ExtMove move = StringToMove(token, searcher_threads_.pool_[0]->board_);
        if (move.GetFrom() == move.GetTo()) { return; }
        moving::MakeMove(searcher_threads_.pool_[0]->board_, move);
    }
}

void UCI::ExecuteSearch()
{
    while (true)
    {
        std::unique_lock<std::mutex> lck(search_mutex);
        search_args_cv.wait(lck, [&](){ return (search_requested_ || quit_flag_); });

        if (quit_flag_) { break; } 

        search_requested_ = false;

        std::string args_copy = std::move(search_args);
        lck.unlock();
        std::istringstream stream(args_copy);
        std::string token;
    
        int64_t wtime = 1000;
        int64_t btime = 1000;
        int64_t winc = 100;
        int64_t binc = 100;
    
        int64_t movetime{};      // 0 when absent
        int64_t depth_arg{};     // 0 when absent
        bool has_clock{};        // did the GUI give us a clock at all?
    
        while (stream >> token)
        {
            if (token == "wtime") { stream >> wtime; has_clock = true; }
            else if (token == "btime") { stream >> btime; has_clock = true; }
            else if (token == "winc") { stream >> winc; has_clock = true; }
            else if (token == "binc") { stream >> binc; has_clock = true; }
            else if (token == "movetime") { stream >> movetime; }
            else if (token == "depth") { stream >> depth_arg; }
        }
    
        size_t max_depth = (depth_arg > 0) ? std::min<size_t>(static_cast<size_t>(depth_arg), kMaxSearchDepth) : kMaxSearchDepth;
    
        int64_t soft_ms{};
        int64_t hard_ms{};
    
        if ((!has_clock && movetime <= 0))
        {
            // "go infinite", and "go depth N" with no clock: run until done or stopped.
            soft_ms = kNoTimeLimit;
            hard_ms = kNoTimeLimit;
        }
        else if (movetime > 0)
        {
            // Soft == hard so the predictive break in FindBestMove stops us starting
            // an iteration we can't finish, instead of the hard cutoff truncating one.
            soft_ms = movetime;
            hard_ms = movetime;
        }
        else
        {
            int64_t my_time = (searcher_threads_.pool_[0]->board_.GetCurStatus().cur_color_ == Color::kWhite) ? wtime : btime;
            int64_t my_inc = (searcher_threads_.pool_[0]->board_.GetCurStatus().cur_color_ == Color::kWhite) ? winc  : binc;
    
            soft_ms = (my_time / 20) + (my_inc / 2); // per thread.
            hard_ms = std::min<int64_t>(soft_ms * 4, my_time / 3);
        }
        
        
        {
            for (size_t i{}; i < searcher_threads_.thread_count_; i++)
            {
                searcher_threads_.pool_[i]->best_move = ExtMove{}; // reset best_move;
                searcher_threads_.pool_[i]->search_args.depth = max_depth;
                searcher_threads_.pool_[i]->search_args.soft_ms = soft_ms;
                searcher_threads_.pool_[i]->search_args.start_depth = 1 + (i % 4) * 2;

                if (i > 0)
                {
                    searcher_threads_.pool_[i]->board_ = searcher_threads_.pool_[0]->board_; // copy board for all threads.
                }
            }
        }

        searcher_threads_.StartSearch();
        std::unordered_map<uint16_t, int> votes{};

        {
            std::unique_lock lck(searcher_threads_.mtx_);
            auto done = [&]{
                for (auto& thrd : searcher_threads_.pool_) 
                { 
                    if (thrd->searching_) { return false; }
                }
                return true;
            };

            if (hard_ms == kNoTimeLimit)
            {
                searcher_threads_.cv_.wait(lck, done);
            }
            else if (!searcher_threads_.cv_.wait_for(lck, std::chrono::milliseconds(hard_ms), done)) 
            {
                searcher_threads_.stop_flag_ = true;
                searcher_threads_.cv_.wait(lck, done);
            }
        }
    
        for (auto& thrd : searcher_threads_.pool_)
        {
            votes[thrd->best_move.Raw()]++;
        }   

        auto winner_vote = std::max_element(votes.begin(), votes.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
        ExtMove best_move{Move{winner_vote->first}};   // adapt to your ctor

        std::cout << "info number of votes " << winner_vote->second << "\n";
        std::cout << "bestmove " << ((best_move.GetFrom() == best_move.GetTo()) ? "0000" : MoveToString(best_move)) << "\n";
    }
}

// command worker thread, will wait until info comes into the terminal and add it to the queue.
// Reads the terminal. Runs on the main thread.
void UCI::Loop()
{
    std::cout << std::unitbuf; 
    std::thread search_manager(&UCI::ExecuteSearch, this);

    std::string line;
    while (std::getline(std::cin, line))
    {
        if (!line.empty() && line.back() == '\r') { line.pop_back(); }

        if (line.empty()) { continue; }

        // Bypass the queue: a queued "stop" would sit behind the "go" it means to interrupt.
        if (line == "stop") { searcher_threads_.stop_flag_ = true; continue; }

        if (line == "quit") 
        {
            searcher_threads_.stop_flag_ = true;
            { 
                std::lock_guard lck(search_mutex); 
                quit_flag_ = true; 
            }

            search_args_cv.notify_one();
            break;
        }
    
        size_t split = line.find(' ');
        std::string cmd = line.substr(0, split);
        std::string args = (split == std::string::npos) ? std::string{} : line.substr(split + 1);

        if (cmd == "uci")
        {
            std::cout << "id name coffee_chess_engine\n"
                        << "id author Grisha\n"
                        << "option name Hash type spin default 128 min 1 max 1024\n"
                        << "option name Threads type spin default 6 min 1 max 8\n"
                        << "uciok\n";
        }
        else if (cmd == "setoption")
        {
            SetOption(args);
        }
        else if (cmd == "isready")
        {
            std::cout << "readyok\n";
        }
        else if (cmd == "ucinewgame")
        {
            searcher_threads_.pool_[0]->board_.ResetBoard();
            for (auto& thrd : searcher_threads_.pool_)
            {
                thrd->searcher_.ResetSearch();
            }
        }
        else if (cmd == "position")
        {
            SetPosition(args);
        }
        else if (cmd == "go")
        {
            {
                std::unique_lock<std::mutex> lck(search_mutex);
                search_args = std::move(args);
                search_requested_ = true;
                lck.unlock();
                search_args_cv.notify_one();
            }
        }
    }

    searcher_threads_.Quit();
    search_manager.join();
}


void UCI::SetOption(const std::string& args)
{
    std::istringstream stream(args);
    std::string token;
    std::string name;
    std::string value;

    if (!(stream >> token) || token != "name") { return; }

    // Names may contain spaces, so everything up to "value" is the name.
    while (stream >> token && token != "value")
    {
        if (!name.empty()) { name += ' '; }
        name += token;
    }
    while (stream >> token)
    {
        if (!value.empty()) { value += ' '; }
        value += token;
    }

    for (char& c : name) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }

    if (name == "hash")
    {
        searcher_threads_.pool_[0]->searcher_.ttable_->Resize(std::stoul(value)); 
    }
    if (name == "threads")
    {
        searcher_threads_.Resize(std::stoul(value));
    }
}