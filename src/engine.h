#pragma once
#include <chrono>
#include <string>
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
    perft
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
    std::atomic_bool b_stop;
    PVTable pv;
};

class Engine
{
private:
    BitBoard m_board = BitBoard::start_position();
    std::string m_uci;
    std::string m_algebraic;
    std::vector<std::string> m_pv;
    int m_evaluation;

    template <side_t Side>
    static auto search(search_args args, search_state &state) -> int;

    template <side_t Side>
    static void search_async(search_state &state);

    void convert_pv(const PVTable &pv_table);
    static auto move_to_uci(const Move &move, const BitBoard &board) -> std::string;
    auto move_to_uci(const Move &move) -> std::string { return move_to_uci(move, m_board); };
    static auto move_to_algebraic(const Move &move, BitBoard board) -> std::string;
    auto move_to_algebraic(const Move &move) -> std::string
    {
        return move_to_algebraic(move, m_board);
    };

    auto handle_position(const std::string &token) -> bool;
    auto handle_go(const std::string &type_str, const std::string &value_str) -> bool;
    static auto split_into_tokens(const std::string &str) -> std::vector<std::string>;

public:
    void uci_loop();
    static auto bitboard_to_string(const uint64_t &board) -> std::string;
    void run(std::chrono::seconds timeout);
    void run(int depth);
    void load(const std::string &fen);

    auto get_uci() -> const std::string &;
    auto get_algebraic() -> const std::string &;
};
