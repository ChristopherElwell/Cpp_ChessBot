#pragma once
#include "search.h"
#include "helpers.h"
#include "move_gen.h"
#include "bitboard.h"
#include <string>
#include <queue>
#include <mutex>
#include <vector>
#include <thread>
#include <atomic>

namespace control
{
    std::string get_bot_move_at_depth_uci(std::string FEN, int max_depth);
    std::string get_bot_move_at_depth_algebreic(std::string FEN, int max_depth);
    void receiver();

    class ThreadPool
    {
    public:
        ThreadPool(BitBoard &bb, int num_threads, int time_ms);
        ~ThreadPool();
        std::string invoke();

    private:
        BitBoard &origin_board;
        int num_threads;
        int time_ms;
        std::vector<std::thread> workers;
        std::vector<BitBoard> boards;
        std::array<Move, MOVES_ARRAY_LENGTH> movs;
        std::mutex jobs_mutex;
        std::atomic<bool> stop = false;
        int depth = 1;
        size_t index = 0;
        std::vector<std::vector<SearchRes *>> results;

        void worker();
        std::pair<size_t, BitBoard *> grab_next_board();
    };
}
