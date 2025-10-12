#include "engine.h"

#include <chrono>
#include <climits>
#include <iostream>
#include <limits>
#include <print>
#include <string>

#include "bitboard.h"
#include "move_gen.h"
#include "search.h"

using namespace std;

void Engine::run(int depth)
{
    unique_ptr<const result_t> p_result;
    if (m_board.whites_turn())
    {
        std::tie(m_evaluation, p_result) = search<side_t::white>(
            m_board, depth, numeric_limits<int>::min(), numeric_limits<int>::max());
    }
    else
    {
        std::tie(m_evaluation, p_result) = search<side_t::black>(
            m_board, depth, numeric_limits<int>::min(), numeric_limits<int>::max());
    }
    if (p_result)
    {
        compute_results(p_result);
    }
    else
    {
        print("ERROR: Result is nullptr\n``");
    }
}

void Engine::run(chrono::seconds timeout)
{
    auto deadline = chrono::steady_clock::now() + timeout;
    int depth = 1;
    unique_ptr<const result_t> p_result;
    while (chrono::steady_clock::now() < deadline)
    {
        if (m_board.whites_turn())
        {
            std::tie(m_evaluation, p_result) = search<side_t::white>(
                m_board, depth, numeric_limits<int>::min(), numeric_limits<int>::max());
        }
        else
        {
            std::tie(m_evaluation, p_result) = search<side_t::black>(
                m_board, depth, numeric_limits<int>::min(), numeric_limits<int>::max());
        }
    }
    if (p_result)
    {
        compute_results(p_result);
    }
    else
    {
        print("ERROR: Result is nullptr\n");
    }
}

void Engine::compute_results(const unique_ptr<const result_t>& p_result)
{
    m_algebraic = move_to_algebraic(p_result->best_move);
    m_uci = move_to_uci(p_result->best_move);
    fill_pv(p_result);
}

auto Engine::get_uci() -> const string& { return m_uci; }

auto Engine::get_algebraic() -> const string& { return m_algebraic; }

void Engine::load(const string& FEN) { m_board = BitBoard(FEN); }

void Engine::fill_pv(const unique_ptr<const result_t>& p_result)
{
    m_pv.clear();
    const result_t* node = p_result.get();
    BitBoard board = m_board;
    while (node != nullptr)
    {
        m_pv.push_back(move_to_uci(node->best_move, board));
        board.apply_move(node->best_move);
        if (!node->next)
        {
            break;
        }
        node = node->next.get();
    }
}

auto Engine::move_to_uci(const Move& mov, const BitBoard& board) -> string
{
    string out;
    uint64_t starting_sq = 0;
    uint64_t ending_sq = 0;

    switch (mov.type)
    {
        case movType::QUIET:
        case movType::CAPTURE:
        case movType::CASTLE_kingside:
        case movType::CASTLE_queenside:
            starting_sq = mov.mov1 & board[mov.pc1];
            ending_sq = mov.mov1 & ~board[mov.pc1];
            out += square_coords[__builtin_ctzll(starting_sq)];
            out += square_coords[__builtin_ctzll(ending_sq)];
            return out;
        case movType::PROMOTE:
            starting_sq = mov.mov1 & board[mov.pc1];
            ending_sq = mov.mov2;
            out += square_coords[__builtin_ctzll(starting_sq)];
            out += square_coords[__builtin_ctzll(ending_sq)];
            out += piece_chars[static_cast<size_t>(mov.pc2)];
            return out;
        case movType::CAPTURE_PROMOTE:
            starting_sq = mov.mov1 & board[mov.pc1];
            ending_sq = mov.mov3;
            out += square_coords[__builtin_ctzll(starting_sq)];
            out += square_coords[__builtin_ctzll(ending_sq)];
            out += piece_chars[static_cast<size_t>(mov.pc3)];
            return out;
        case movType::BOOK_END:
            return "BOOK END";
        default:
            return "UNKNOWN";
    }
    return out;
}

auto Engine::move_to_algebraic(const Move& move, BitBoard board) -> string
{
    const uint64_t to_pos = move.mov1 & ~board[move.pc1];
    const uint64_t from_pos = move.mov1 & ~to_pos;
    string out;
    string destination = square_coords[__builtin_ctzll(to_pos)];
    string origin = square_coords[__builtin_ctzll(from_pos)];
    if (move.pc1 == piece_t::white_pawn || move.pc1 == piece_t::black_pawn)
    {
        switch (move.type)
        {
            case movType::QUIET:
                out = destination;
                break;
            case movType::CAPTURE:
                out = format("{}x{}", origin[0], destination);
                break;
            case movType::PROMOTE:
                out = format("{}{}", destination, piece_chars[static_cast<int>(move.pc2) % 6]);
                break;
            case movType::CAPTURE_PROMOTE:
                out = format("{}{}", origin[0], square_coords[__builtin_ctzll(move.mov2)],
                             piece_chars[static_cast<int>(move.pc3) % 6]);
                break;
            case movType::CASTLE_kingside:
            case movType::CASTLE_queenside:
            default:
                return "Unknown";
                break;
        }
    }
    else
    {
        switch (move.type)
        {
            case movType::QUIET:
                out = string(1, piece_chars[static_cast<int>(move.pc1) % 6]) + destination;
                break;
            case movType::CAPTURE:

                out = string(1, piece_chars[static_cast<int>(move.pc1) % 6]) + "x" + destination;
                break;
            case movType::CASTLE_kingside:
                out = "O-O";
                break;
            case movType::CASTLE_queenside:
                out = "O-O-O";
                break;
            case movType::CAPTURE_PROMOTE:
            default:
                return "Unknown";
        }
    }
    auto move_gen = MoveGen(board);
    board.apply_move(move);
    if (board.whites_turn())
    {
        if (move_gen.is_king_in_check<side_t::white>())
        {
            out += "+";
        }
    }
    else
    {
        if (move_gen.is_king_in_check<side_t::black>())
        {
            out += "+";
        }
    }
    board.apply_move(move);
    return out;
}

auto Engine::bitboard_to_string(const uint64_t& board) -> string
{
    string out;
    for (int i = 0; i < 64; i++)
    {
        if (i % 8 == 0)
        {
            out += format("{} ", 8 - (i / 8));
        }
        if ((board & (1ULL << (63 - i))) != 0)
        {
            out += white_sq_char;
        }
        else
        {
            out += black_sq_char;
        }
        if ((i + 1) % 8 == 0)
        {
            out += "\n";
        }
    }
    return out + "  a b c d e f g h\n\n";
}