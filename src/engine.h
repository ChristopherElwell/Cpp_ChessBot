#pragma once
#include <string>
#include <chrono>
#include <vector>

#include "bitboard.h"
#include "move.h"

struct result_t
{
    Move best_move;
    std::unique_ptr<const result_t> next;
};

class Engine{
private:
    BitBoard m_board = BitBoard::start_position();
    std::string m_uci;
    std::string m_algebraic;
    std::vector<std::string> m_pv;
    int m_evaluation;

    template<side_t side>
    static auto search(BitBoard &board, int iter, int alpha, int beta) 
    -> std::pair<int,std::unique_ptr<result_t>>;
    void fill_pv(const std::unique_ptr<const result_t>& p_result);
    static auto move_to_uci(const Move &move, const BitBoard& board) -> std::string;
    auto move_to_uci(const Move &move) -> std::string {
        return move_to_uci(move,m_board);
    };
    static auto move_to_algebraic(const Move &move, BitBoard board) -> std::string;
    auto move_to_algebraic(const Move &move) -> std::string {
        return move_to_algebraic(move,m_board);
    };
    void compute_results(const std::unique_ptr<const result_t>& p_result);
    
public:
    static auto bitboard_to_string(const uint64_t &board) -> std::string;
    void run(std::chrono::seconds timeout);
    void load(const std::string& FEN);
    void run(int depth);

    auto get_uci() -> const std::string&;
    auto get_algebraic() -> const std::string&;
};

