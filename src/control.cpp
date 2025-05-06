#include "control.h"
#include "search.h"
#include "helpers.h"
#include "move.h"
#include "move_gen.h"
#include "bitboard.h"
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#include <time.h>
#include <utility>
#include <algorithm>

namespace control
{

    std::string get_bot_move_at_depth_uci(std::string FEN, int max_depth)
    {
        BitBoard bb = BitBoard(FEN);
        MoveGen mg;
        SearchRes *result = search(bb, mg, max_depth, INT16_MIN, INT16_MAX);
        helpers::print_principal_variation(result, bb);
        std::string best = helpers::move_to_uci(result->best_move, bb);
        helpers::free_search_result(result);
        return best;
    }

    std::string get_bot_move_at_depth_algebreic(std::string FEN, int max_depth)
    {
        BitBoard bb = BitBoard(FEN);
        MoveGen mg;
        SearchRes *result = search(bb, mg, max_depth, INT16_MIN, INT16_MAX);
        helpers::print_principal_variation(result, bb);
        std::string best = helpers::move_to_algebreic(result->best_move, bb);
        helpers::free_search_result(result);
        return best;
    }

    void receiver()
    {
        std::string line;
        while (std::getline(std::cin, line))
        {
            std::string move = get_bot_move_at_depth_uci(line, 5);
            std::cerr << move << std::endl; // Output response
            std::cerr.flush();              // Ensure Python gets it immediately
        }
    }

    ThreadPool::ThreadPool(BitBoard &bb, int num_threads, int time_ms)
        : origin_board(bb), num_threads(num_threads), time_ms(time_ms)
    {
        MoveGen mg;
        mg.get_moves(this->movs, bb);
        for (Move m : this->movs)
        {
            if (m.type == movType::BOOK_END)
                break;
            BitBoard bb_copy = bb;
            bb_copy.apply_move(m);
            this->boards.push_back(bb_copy);
        }
        results.push_back(std::vector<SearchRes *>());
        results[0].resize(this->boards.size());
    }

    std::string ThreadPool::invoke()
    {
        for (int i = 0; i < this->num_threads; i++)
        {
            this->workers.emplace_back(&ThreadPool::worker, this);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(this->time_ms));
        this->stop = true;
        for (std::thread &t : this->workers)
            t.join();

        auto rit = std::find_if(this->results.rbegin(), this->results.rend(),
                                [](const std::vector<SearchRes *> &inner)
                                {
                                    return std::all_of(inner.begin(), inner.end(), [](SearchRes *ptr)
                                                       { return ptr != nullptr; });
                                });

        int completed_index = std::distance(rit, this->results.rend()) - 1;
        std::cout << "Best result came from iteration " << completed_index << "\n";

        if (rit == this->results.rend())
        {
            std::cout << "NO ITER COMPLETED";
            return "ERROR";
        }

        std::vector<SearchRes *> &last_full_result = *rit;

        auto best_eval_lambda = (this->origin_board[Piece::INFO] & TURN_BIT)
                                    ? [](const SearchRes *a, const SearchRes *b)
        { return a->best_eval < b->best_eval; }
                                    : [](const SearchRes *a, const SearchRes *b)
        { return a->best_eval > b->best_eval; };

        auto best = std::max_element(last_full_result.begin(), last_full_result.end(), best_eval_lambda);

        SearchRes *best_result = (best != last_full_result.end()) ? *best : nullptr;
        helpers::print_principal_variation(best_result, this->origin_board);
        int best_result_idx = std::distance(last_full_result.begin(), best);
        return helpers::move_to_uci(this->movs[best_result_idx], this->origin_board);
    }

    std::pair<size_t, BitBoard *> ThreadPool::grab_next_board()
    {
        if (this->index >= this->boards.size())
        {
            this->index = 0;
            this->depth++;
            std::cout << depth << ", ";
            this->results.push_back(std::vector<SearchRes *>());
            this->results[this->depth - 1].resize(this->boards.size());
        }
        size_t current_index = this->index++;
        return std::make_pair(current_index, &this->boards[current_index]);
    }

    void ThreadPool::worker()
    {
        MoveGen mg;
        while (!this->stop)
        {
            std::pair<size_t, BitBoard *> task_info;
            int depth;
            {
                std::lock_guard<std::mutex> lock(this->jobs_mutex);
                task_info = grab_next_board();
                depth = this->depth;
            }

            SearchRes *result = search(*task_info.second, mg, depth, INT16_MIN, INT16_MAX);

            {
                std::lock_guard<std::mutex> lock(this->jobs_mutex);
                results[depth - 1][task_info.first] = result;
            }
        }
    }

    ThreadPool::~ThreadPool()
    {
        for (auto outer : this->results)
        {
            for (SearchRes *sr : outer)
            {
                helpers::free_search_result(sr);
            }
        }
    }

}