#pragma once

#include "board.hpp"
#include "move.hpp"
#include "search.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <future>
#include <iostream>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>


inline std::string MoveToString(ExtMove move)
{
    
    std::string move_str;
    move_str.reserve(5);
    move_str += SquareToString(move.GetFrom());
    move_str += SquareToString(move.GetTo());

    if (move.IsPromotion())
    {
        switch (move.GetPromo()) 
        {
            case Piece::kQueen: move_str += "q"; break; 
            case Piece::kRook: move_str += "r"; break; 
            case Piece::kBishop: move_str += "b"; break; 
            case Piece::kKnight: move_str += "n"; break; 
        }
    }
   return move_str;
}


struct ThreadSearchArgs
{
    uint64_t soft_ms{};
    size_t depth{};
    size_t start_depth{};
    uint64_t nodes{};        // 0 = no node limit
};

class ThreadPool;

class SearcherThread 
{
public:
    SearcherThread(int id, std::atomic<bool>& stop_flag, std::shared_ptr<TTable> tt)
        : id_(id), searcher_(stop_flag, std::move(tt)) {}

    int id_;
    std::thread thread_;

    Search searcher_;
    Board board_;
    ThreadSearchArgs search_args{};
    ExtMove best_move;

    bool searching_ = false;          // "go" flag, per thread, guarded by pool mutex
    void IdleLoop(ThreadPool& pool);
};



class ThreadPool 
{
public:
    size_t thread_count_;
    
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> stop_flag_{false};   // checked inside search loop
    bool quit_flag_ = false;               // guarded by mtx_
    
    std::vector<std::unique_ptr<SearcherThread>> pool_;
    std::shared_ptr<TTable> ttable_ = std::make_shared<TTable>();
    
    void Launch(size_t thread_count)
    {
        thread_count_ = (thread_count == 0) ? 1 : thread_count;

        pool_.reserve(thread_count_);
        for (size_t i{}; i < thread_count_; ++i)
        {
            pool_.push_back(std::make_unique<SearcherThread>(
                static_cast<int>(i), stop_flag_, ttable_));
        }
        for (auto& thrd : pool_)
        {
            thrd->thread_ = std::thread(&SearcherThread::IdleLoop, thrd.get(), std::ref(*this));
        }
    }

    void Resize(size_t thread_count)
    {
        Quit();                 // stops and joins every existing thread
        pool_.clear();          // destroys the SearcherThreads (threads already joined)

        {
            std::lock_guard lck(mtx_);
            quit_flag_ = false;
            stop_flag_ = false;
        }

        Launch(thread_count);
    }


    void StartSearch() 
    {
        { 
            std::lock_guard lck(mtx_);
            if (!quit_flag_) { stop_flag_ = false; }
            
            for (auto& thrd : pool_) 
            {
                thrd->searching_ = true;
            }
        }

        cv_.notify_all();
    }

    void Quit() 
    {
        { 
            std::lock_guard lck(mtx_); 
            quit_flag_ = true; 
            stop_flag_ = true;
        }

        cv_.notify_all();
        for (auto& thread : pool_) 
        { 
            if (thread->thread_.joinable())
            {
                thread->thread_.join(); 
            } 
        }
    }

    explicit ThreadPool(size_t thread_count)
    {
        Launch(thread_count);
    }

    ~ThreadPool()
    {
        Quit();
    }
};



class UCI
{
    bool search_requested_{false};
    ThreadPool searcher_threads_{3}; // thread 0 will always be avaiable for search. its the main searcher thread.

    std::condition_variable search_args_cv; // starting the search
    std::mutex search_mutex; // locks args
    std::string search_args;
    bool quit_flag_ = false;

    void SetOption(const std::string& args);
    void SetPosition(const std::string& input);
    void ExecuteSearch();

public:
    
    explicit UCI(size_t threads) : searcher_threads_(threads) {};
    void Loop();
};
