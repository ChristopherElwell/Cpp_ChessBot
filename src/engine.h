#pragma once
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "bitboard.h"
#include "move.h"
#include "pv.h"

#ifdef _DEBUG
#define DEBUG_LOG(...) std::println(std::cerr, __VA_ARGS__)
#else
#define DEBUG_LOG(...) ((void)0)
#endif
#define LOG(...) std::println(std::cerr, __VA_ARGS__)

enum class mode : uint8_t
{
    uci,
    puzzles,
    perft,
    conversion
};

struct arguments
{
    mode mode = mode::uci;
    int count = 0;
};

struct result_t
{
    Move best_move;
    std::unique_ptr<const result_t> next;
};

struct search_args
{
    int depth;
    int alpha;
    int beta;
    int ply;
};

struct search_state
{
    BitBoard board;
    std::atomic_bool *b_stop = nullptr;
    PVTable pv;
};

class Engine
{
private:
    BitBoard m_board = BitBoard::start_position();
    std::string m_uci;
    std::string m_algebraic;
    std::vector<std::string> m_pv_uci;
    int m_evaluation;
    std::atomic_bool m_b_stop;
    std::chrono::high_resolution_clock::time_point m_stop_time;
    std::thread m_search_thread;
    std::thread m_timer_thread;
    std::mutex m_stop_cv_lock;
    std::condition_variable m_stop_cv;
    std::mutex m_search_lock;
    static constexpr auto max_search_time = std::chrono::minutes{5};

    template <side_t Side>
    static auto search(search_args args, search_state &state) -> int;

    template <side_t Side>
    void search_async();
    template <side_t Side>
    void search_async(int depth);

    void convert_pv(const PVTable &pv_table);
    auto move_to_uci(const Move &move) -> std::string { return move_to_uci(move, m_board); };
    auto move_to_algebraic(const Move &move) -> std::string
    {
        return move_to_algebraic(move, m_board);
    };
    auto uci_to_move(const std::string &uci) -> Move { return uci_to_move(uci, m_board); }

    auto parse_and_set_position(const std::string &message) -> bool;
    auto parse_run(const std::string &message) -> bool;
    static auto split_into_tokens(const std::string &str) -> std::vector<std::string>;

public:
    void uci_loop();
    static auto bitboard_to_string(const uint64_t &board) -> std::string;
    static auto uci_to_move(const std::string &uci, BitBoard &board) -> Move;
    static auto move_to_uci(const Move &move, const BitBoard &board) -> std::string;
    static auto move_to_algebraic(const Move &move, BitBoard &board) -> std::string;
    void run(std::chrono::milliseconds duration = max_search_time);
    void run(int depth);
    void load(const std::string &fen);

    auto get_uci() -> const std::string &;
    auto get_algebraic() -> const std::string &;
};
