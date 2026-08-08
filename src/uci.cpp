#include "uci.hpp"
#include "debug.hpp"
#include "eval_tuning.hpp"
#include "global.hpp"
#include "move.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include "move_making.hpp"


inline Square StringToSquare(const std::string& sqr_str) 
{
    if (sqr_str.length() != 2) return Square::kNoSquare;
    
    Column col = static_cast<Column>(sqr_str[0] - 'a');
    Row row = static_cast<Row>(sqr_str[1] - '1');
    
    if (row > Row::_8 || col > Column::H) return Square::kNoSquare;
    
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


void UCI::SetPosition(const std::string& input)
{
    std::istringstream stream(input);
    std::string token;

    if (!(stream >> token)) { return; }

    if (token == "startpos")
    {
        board_.ResetBoard();
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
        board_.ResetBoard(fen);
    }
    else
    {
        return;
    }

    if (token != "moves") { return; }

    while (stream >> token)
    {
        ExtMove move = StringToMove(token, board_);
        if (move.GetFrom() == move.GetTo()) { return; }
        moving::MakeMove(board_, move);
    }
}


void UCI::ExecuteSearch(const std::string& args)
{
    std::istringstream stream(args);
    std::string token;

    int64_t wtime = 1000;
    int64_t btime = 1000;
    int64_t winc = 100;
    int64_t binc = 100;

    int64_t movetime{};      // 0 when absent
    int64_t depth_arg{};     // 0 when absent
    bool infinite{};
    bool has_clock{};        // did the GUI give us a clock at all?

    while (stream >> token)
    {
        if (token == "wtime") { stream >> wtime; has_clock = true; }
        else if (token == "btime") { stream >> btime; has_clock = true; }
        else if (token == "winc") { stream >> winc; has_clock = true; }
        else if (token == "binc") { stream >> binc; has_clock = true; }
        else if (token == "movetime") { stream >> movetime; }
        else if (token == "depth") { stream >> depth_arg; }
        else if (token == "infinite") { infinite = true; }
    }

    size_t max_depth = (depth_arg > 0)
        ? std::min<size_t>(static_cast<size_t>(depth_arg), kMaxSearchDepth)
        : kMaxSearchDepth;


    constexpr int64_t kNoTimeLimit = std::numeric_limits<int64_t>::max() / 4;

    int64_t soft_ms{};
    int64_t hard_ms{};

    if (infinite || (!has_clock && movetime <= 0))
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
        int64_t my_time = (board_.GetCurStatus().cur_color_ == Color::kWhite) ? wtime : btime;
        int64_t my_inc  = (board_.GetCurStatus().cur_color_ == Color::kWhite) ? winc  : binc;

        int64_t allot = (my_time / 20) + (my_inc / 2);
        soft_ms = allot;
        hard_ms = std::min<int64_t>(allot * 4, my_time / 3);
    }

    stop_flag = false;
    bool search_done = false;

    ExtMove best_move{};
    std::thread search_thread([&]
    {
        ExtMove result = searcher_.FindBestMove(soft_ms, max_depth);
        {
            std::lock_guard<std::mutex> lock(board_mutex);
            best_move = result;
            search_done = true;
        }
        search_cv.notify_one();
    });

    {
        std::unique_lock<std::mutex> lock(board_mutex);
        if (hard_ms >= kNoTimeLimit)
        {
            search_cv.wait(lock, [&] { return search_done; });
        }
        else
        {
            search_cv.wait_for(lock, std::chrono::milliseconds(hard_ms),
                                [&] { return search_done; });
        }
    }

    // Whether we woke from the timeout or from the search finishing, cutting it off is safe.
    stop_flag = true;
    search_thread.join();

    std::cout << "bestmove " << ((best_move.GetFrom() == best_move.GetTo()) ? "0000" : MoveToString(best_move)) << "\n";
}


void UCI::CommandWorker()
{
    while (true)
    {
        std::string line;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, [this] { return !cmd_queue.empty() || quit_flag; });

            if (quit_flag) { break; }

            line = std::move(cmd_queue.front());
            cmd_queue.pop();
        }

        size_t split = line.find(' ');
        std::string cmd = line.substr(0, split);
        std::string args = (split == std::string::npos) ? std::string{} : line.substr(split + 1);

        if (cmd == "uci")
        {
            std::cout << "id name coffee_chess_engine\n"
                      << "id author Grisha\n"
                      << "option name Hash type spin default " << kDefaultHashMB << " min 1 max 4096\n";
                      eval_tuning::PrintOptions(std::cout);
                      std::cout << "uciok\n";
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
            board_.ResetBoard();
            searcher_.ResetSearch();
        }
        else if (cmd == "position")
        {
            SetPosition(args);
        }
        else if (cmd == "go")
        {
            ExecuteSearch(args);
        }
        else if (cmd == "history")
        {
            searcher_.PrintHistoryStats();
        }
        else if (cmd == "spsa")
        {
            // Not UCI: dumps the weather-factory config for the eval tunables.
            eval_tuning::PrintSpsaConfig(std::cout);
        }
    }
}

// command worker thread, will wait until info comes into the terminal and add it to the queue.
// Reads the terminal. Runs on the main thread.
void UCI::Loop()
{
    std::cout << std::unitbuf; 
    std::thread cmd_thread(&UCI::CommandWorker, this);

    std::string line;
    while (std::getline(std::cin, line))
    {
        if (!line.empty() && line.back() == '\r') { line.pop_back(); }
        if (line.empty()) { continue; }

        // Bypass the queue: a queued "stop" would sit behind the "go" it means to interrupt.
        if (line == "stop") { stop_flag = true; continue; }

        if (line == "quit") { break; }

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            cmd_queue.push(std::move(line));
        }

        queue_cv.notify_one();
    }

    stop_flag = true;
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        quit_flag = true;
    }
    queue_cv.notify_one();
    cmd_thread.join();
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
        try { searcher_.ttable_->Resize(std::stoul(value)); }
        catch (const std::exception&) { }   // malformed value: keep the current size
        return;
    }

    // Eval tunables. Unknown names fall through and are ignored, as UCI expects.
    try { eval_tuning::SetParam(name, std::stoi(value)); }
    catch (const std::exception&) { }   // malformed value: keep the current weight
}