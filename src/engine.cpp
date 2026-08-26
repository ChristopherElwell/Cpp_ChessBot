#include "engine.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <iostream>
#include <limits>
#include <memory>
#include <print>
#include <ranges>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include "bitboard.h"
#include "data.h"
#include "move.h"
#include "move_gen.h"

using namespace std;

void Engine::run(int depth)
{
    unique_ptr<const result_t> p_result;
    atomic_bool b_stop = false;
    if (m_board.whites_turn())
    {
        tie(m_evaluation, p_result) = search<side_t::white>(
            m_board, depth, numeric_limits<int>::min(), numeric_limits<int>::max(), 0, b_stop);
    }
    else
    {
        tie(m_evaluation, p_result) = search<side_t::black>(
            m_board, depth, numeric_limits<int>::min(), numeric_limits<int>::max(), 0, b_stop);
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
    atomic_bool b_stop = false;

    pair<int, unique_ptr<const result_t>> result = {0, nullptr};

    thread search_thread(
        [this, &b_stop, &result]() -> void
        {
            if (m_board.whites_turn())
            {
                search_async<side_t::white>(result, m_board, b_stop);
            }
            else
            {
                search_async<side_t::black>(result, m_board, b_stop);
            }
        });

    this_thread::sleep_for(timeout);
    b_stop.store(true, memory_order_relaxed);
    search_thread.join();

    if (result.second)
    {
        compute_results(result.second);
    }
    else
    {
        print("ERROR: Result is nullptr\n");
    }
}

void Engine::compute_results(const unique_ptr<const result_t>& p_result)
{
    m_uci = move_to_uci(p_result->best_move);
    m_algebraic = move_to_algebraic(p_result->best_move);
    fill_pv(p_result);
}

auto Engine::get_uci() -> const string& { return m_uci; }

auto Engine::get_algebraic() -> const string& { return m_algebraic; }

void Engine::load(const string& fen) { m_board = BitBoard(fen); }

void Engine::fill_pv(const unique_ptr<const result_t>& p_result)
{
    m_pv.clear();
    const result_t* p_node = p_result.get();
    BitBoard board = m_board;
    while (p_node != nullptr)
    {
        m_pv.push_back(move_to_uci(p_node->best_move, board));
        board.apply_move(p_node->best_move);
        if (!p_node->next)
        {
            break;
        }
        p_node = p_node->next.get();
    }
}

auto Engine::move_to_uci(const Move& mov, const BitBoard& board) -> string
{
    string out;
    uint64_t starting_sq = 0;
    uint64_t ending_sq = 0;

    switch (mov.type)
    {
        case mov_type::quiet:
        case mov_type::capture:
        case mov_type::castle_kingside:
        case mov_type::castle_queenside:
            starting_sq = mov.mov1 & board[mov.pc1];
            ending_sq = mov.mov1 & ~board[mov.pc1];
            out += square_coords.at(countr_zero(starting_sq));
            out += square_coords.at(countr_zero(ending_sq));
            return out;
        case mov_type::promote:
            starting_sq = mov.mov1 & board[mov.pc1];
            ending_sq = mov.mov2;
            out += square_coords.at(countr_zero(starting_sq));
            out += square_coords.at(countr_zero(ending_sq));
            out += lower_case_piece_chars.at(static_cast<size_t>(mov.pc2) % 6);
            return out;
        case mov_type::capture_promote:
            starting_sq = mov.mov1 & board[mov.pc1];
            ending_sq = mov.mov3;
            out += square_coords.at(countr_zero(starting_sq));
            out += square_coords.at(countr_zero(ending_sq));
            out += piece_chars.at(static_cast<size_t>(mov.pc3));
            return out;
        case mov_type::moves_termination:
            return "MOVES TERMINATED";
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
    if (move.pc1 == piece_t::white_pawn || move.pc1 == piece_t::black_pawn)
    {
        switch (move.type)
        {
            case mov_type::quiet:
                out = square_coords.at(countr_zero(to_pos));
                break;
            case mov_type::capture:
                out = format("{}x{}", square_coords.at(countr_zero(from_pos))[0],
                             square_coords.at(countr_zero(to_pos)));
                break;
            case mov_type::promote:
                out = format("{}{}", square_coords.at(countr_zero(move.mov2)),
                             piece_chars.at(static_cast<int>(move.pc2) % 6));
                break;
            case mov_type::capture_promote:
                out = format("{}x{}={}", square_coords.at(countr_zero(from_pos))[0],
                             square_coords.at(countr_zero(move.mov2)),
                             piece_chars.at(static_cast<int>(move.pc3) % 6));
                break;
            case mov_type::castle_kingside:
            case mov_type::castle_queenside:
            default:
                return "Unknown";
                break;
        }
    }
    else
    {
        switch (move.type)
        {
            case mov_type::quiet:
            {
                const char piece_char = piece_chars.at(static_cast<int>(move.pc1) % 6);
                out = format("{}{}", piece_char, square_coords.at(countr_zero(to_pos)));
                break;
            }
            case mov_type::capture:
            {
                const char piece_char = piece_chars.at(static_cast<int>(move.pc1) % 6);
                out = format("{}x{}", piece_char, square_coords.at(countr_zero(to_pos)));
                break;
            }
            case mov_type::castle_kingside:
                out = "O-O";
                break;
            case mov_type::castle_queenside:
                out = "O-O-O";
                break;
            case mov_type::capture_promote:
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
    for (int i = 0; i < BitBoard::num_squares; i++)
    {
        if (i % 8 == 0)
        {
            out += format("{} ", 8 - (i / 8));
        }
        if ((board & (1ULL << (BitBoard::num_squares - 1 - i))) != 0)
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

auto Engine::split_into_tokens(const string& str) -> vector<string>
{
    vector<string> result;

    auto start = str.begin();

    while (start != str.end())
    {
        // Find opening bracket
        start = std::find(start, str.end(), '[');
        if (start == str.end())
        {
            break;
        }

        start++;

        auto end = std::find(start, str.end(), ']');
        if (end == str.end())
        {
            break;
        }

        // Extract token
        result.emplace_back(start, end);

        start = end + 1;
    }

    return result;
}

auto Engine::handle_position(const string& token) -> bool
{
    if (token == "startpos")
    {
        m_board = BitBoard::start_position();
        return true;
    }
    try
    {
        m_board = BitBoard(token);
    }
    catch (exception& e)
    {
        return false;
    }
    return true;
}

auto Engine::handle_go(const std::string& type_str, const std::string& value_str) -> bool
{
    int value = 0;
    try
    {
        value = stoi(value_str);
    }
    catch (exception& e)
    {
        DEBUG_LOG("Failed at stoi: [{}]", value_str);
        return false;
    }
    if (type_str == "depth")
    {
        DEBUG_LOG("Running at set depth: {}", value);
        run(value);
        DEBUG_LOG("Returning move: {}", m_algebraic);
        DEBUG_LOG("PV: {}",
                  m_pv | std::views::join_with(std::string(", ")) | std::ranges::to<std::string>());
        return true;
    }
    if (type_str == "time")
    {
        DEBUG_LOG("Running at set time: {}s", value);
        run(chrono::seconds(value));
        DEBUG_LOG("Returning move: {}", m_algebraic);
        DEBUG_LOG("PV: {}",
                  m_pv | std::views::join_with(std::string(", ")) | std::ranges::to<std::string>());
        return true;
    }
    return false;
}

void Engine::uci_loop()
{
    string line;

    for (;;)
    {
        getline(cin, line);
        if (line.empty())
        {
            continue;
        }

        auto tokens = split_into_tokens(line);

        DEBUG_LOG("Received {} tokens", tokens.size());
        for (const auto& token : tokens)
        {
            DEBUG_LOG("[{}]", token);
        }

        if (tokens.empty())
        {
            continue;
        }

        if (tokens.at(0) == "ready")
        {
            println("ElwellBot ready");
        }
        else if (tokens.at(0) == "go")
        {
            if (tokens.size() < 4)
            {
                println("Error: go command needs 4 tokens");
                continue;
            }

            if (!handle_position(tokens.at(1)))
            {
                println("Failed to set position");
                continue;
            }
            if (!handle_go(tokens.at(2), tokens.at(3)))
            {
                println("Failed to get best move");
                continue;
            }
            println("bestmove {}", get_uci());
        }
        else
        {
            println("Did not recognize command: {}", tokens.at(0));
        }
    }
}
