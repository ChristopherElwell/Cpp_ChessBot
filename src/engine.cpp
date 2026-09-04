#include "engine.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cassert>
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
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include "bitboard.h"
#include "bitboard_constants.h"
#include "data.h"
#include "eval.h"
#include "move.h"
#include "move_gen.h"
#include "pv.h"

using namespace std;

namespace
{
constexpr int default_moves_left = 30;
constexpr int buffer_ms = 50;
constexpr int min_time_ms = 10;
}  // namespace

auto Engine::get_uci() -> const string& { return m_uci; }

auto Engine::get_algebraic() -> const string& { return m_algebraic; }

void Engine::load(const string& fen) { m_board = BitBoard(fen); }

void Engine::convert_pv(const PVTable& pv_table)
{
    m_pv_uci.clear();
    BitBoard board = m_board;
    for (const auto& mov : pv_table.get_pv_at_ply(0))
    {
        m_pv_uci.push_back(move_to_uci(mov));
        board.apply_move(mov);
    }
}

auto Engine::move_to_uci(const Move& move) -> string
{
    const string_view from_sq = square_coords.at(move.from());
    const string_view to_sq = square_coords.at(move.to());

    switch (move.type())
    {
        case move_type_t::quiet:
        case move_type_t::capture:
        case move_type_t::castle_kingside:
        case move_type_t::castle_queenside:
        case move_type_t::pawn_double:
        case move_type_t::en_passent:
            return format("{}{}", from_sq, to_sq);
        case move_type_t::promote_queen:
            return format("{}{}q", from_sq, to_sq);
        case move_type_t::promote_rook:
            return format("{}{}r", from_sq, to_sq);
        case move_type_t::promote_bishop:
            return format("{}{}b", from_sq, to_sq);
        case move_type_t::promote_knight:
            return format("{}{}n", from_sq, to_sq);
        case move_type_t::capture_promote_queen:
            return format("{}{}q", from_sq, to_sq);
        case move_type_t::capture_promote_rook:
            return format("{}{}r", from_sq, to_sq);
        case move_type_t::capture_promote_bishop:
            return format("{}{}b", from_sq, to_sq);
        case move_type_t::capture_promote_knight:
            return format("{}{}n", from_sq, to_sq);
        case move_type_t::moves_termination:
            return "XXXX";
    }
}

auto Engine::move_to_algebraic(const Move& move, BitBoard& board) -> string
{
    const string_view from_sq = square_coords.at(move.from());
    const string_view to_sq = square_coords.at(move.to());
    const piece_t moving_pc = board.piece_at(move.from());
    const char moving_symbol = piece_t_to_piece_symbol.at(moving_pc);

    string out;
    switch (move.type())
    {
        case move_type_t::quiet:
            if (moving_pc == piece_t::white_pawn || moving_pc == piece_t::black_pawn)
            {
                out = string(to_sq);
            }
            else
            {
                out = format("{}{}", moving_symbol, to_sq);
            }
            break;
        case move_type_t::capture:
            if (moving_pc == piece_t::white_pawn || moving_pc == piece_t::black_pawn)
            {
                out = format("{}x{}", from_sq.at(0), to_sq);
            }
            else
            {
                out = format("{}x{}", moving_symbol, to_sq);
            }
            break;
        case move_type_t::promote_queen:
            out = format("{}=Q", to_sq);
            break;
        case move_type_t::promote_rook:
            out = format("{}=R", to_sq);
            break;
        case move_type_t::promote_bishop:
            out = format("{}=B", to_sq);
            break;
        case move_type_t::promote_knight:
            out = format("{}=N", to_sq);
            break;
        case move_type_t::capture_promote_queen:
            out = format("{}x{}=Q", from_sq.at(0), to_sq);
            break;
        case move_type_t::capture_promote_rook:
            out = format("{}x{}=R", from_sq.at(0), to_sq);
        case move_type_t::capture_promote_bishop:
            out = format("{}x{}=B", from_sq.at(0), to_sq);
        case move_type_t::capture_promote_knight:
            out = format("{}x{}=N", from_sq.at(0), to_sq);
        case move_type_t::castle_kingside:
            out = "O-O";
            break;
        case move_type_t::castle_queenside:
            out = "O-O-O";
            break;
        case move_type_t::pawn_double:
            out = to_sq;
            break;
        case move_type_t::en_passent:
            out = format("{}x{}", from_sq.at(0), to_sq);
            break;
        case move_type_t::moves_termination:
            return "XXXX";
            break;
    }
    const inv_move inverse = board.apply_move(move);
    MoveGen move_gen(board);
    if (board.side_to_move() == side_t::white)
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
    board.undo_move(move, inverse);
}

auto Engine::bitboard_to_string(const uint64_t& board) -> string
{
    string out;
    for (int i = 0; i < num_squares; i++)
    {
        if (i % 8 == 0)
        {
            out += format("{} ", 8 - (i / 8));
        }
        if ((board & (1ULL << (num_squares - 1 - i))) != 0)
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
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    while (iss >> token)
    {
        tokens.push_back(token);
    }
    return tokens;
}

auto Engine::parse_and_set_position(const string& message) -> bool
{
    m_history.clear();
    const auto tokens = split_into_tokens(message);
    if (tokens.at(1) == "startpos")
    {
        m_board = BitBoard::start_position();
    }
    else if (tokens.at(1) == "fen")
    {
        m_board = BitBoard(tokens.at(2));
    }
    else
    {
        DEBUG_LOG(
            "Received faulty position message. Expected either \"fen\" or \"startpos\". Received: "
            "\"{}\"",
            tokens.at(1));
        return false;
    }

    for (const auto& uci : tokens | ranges::views::drop(3))
    {
        const auto mov = uci_to_move(uci);
        m_board.apply_move(mov);
        m_history.push_back(m_board.hash());
    }

    return true;
}

auto Engine::uci_to_move(const string& uci, BitBoard& board) -> Move
{
    const int from_sq = square_coords_to_index.at(uci.substr(0, 2));
    const int to_sq = square_coords_to_index.at(uci.substr(2, 2));

    piece_t from_pc = board.piece_at(from_sq);
    piece_t to_pc = board.piece_at(to_sq);

    assert(from_pc != piece_t::none && "no piece on source square");

    // --- Castling ---
    if (from_pc == piece_t::white_king)
    {
        if (from_sq == countr_zero(castling<side_t::white>::kingside_king_from) &&
            to_sq == countr_zero(castling<side_t::white>::kingside_king_to))
        {
            return {from_sq, to_sq, move_type_t::castle_kingside};
        }
        if (from_sq == countr_zero(castling<side_t::white>::queenside_king_from) &&
            to_sq == countr_zero(castling<side_t::white>::queenside_king_to))
        {
            return {from_sq, to_sq, move_type_t::castle_queenside};
        }
    }
    else if (from_pc == piece_t::black_king)
    {
        if (from_sq == countr_zero(castling<side_t::black>::kingside_king_from) &&
            to_sq == countr_zero(castling<side_t::black>::kingside_king_to))
        {
            return {from_sq, to_sq, move_type_t::castle_kingside};
        }
        if (from_sq == countr_zero(castling<side_t::black>::queenside_king_from) &&
            to_sq == countr_zero(castling<side_t::black>::queenside_king_to))
        {
            return {from_sq, to_sq, move_type_t::castle_queenside};
        }
    }

    // --- Pawn moves: promotion, en passant, double push ---
    if (from_pc == piece_t::white_pawn || from_pc == piece_t::black_pawn)
    {
        const bool is_white = from_pc == piece_t::white_pawn;

        // Promotion: 5th char present, e.g. "e7e8q"
        if (uci.size() == 5)
        {
            bool b_is_capture = to_pc != piece_t::none;
            switch (uci.at(4))
            {
                case 'q':
                    return {from_sq, to_sq,
                            b_is_capture ? move_type_t::capture_promote_queen
                                         : move_type_t::promote_queen};
                case 'r':
                    return {from_sq, to_sq,
                            b_is_capture ? move_type_t::capture_promote_rook
                                         : move_type_t::promote_rook};
                case 'b':
                    return {from_sq, to_sq,
                            b_is_capture ? move_type_t::capture_promote_bishop
                                         : move_type_t::promote_bishop};
                case 'n':
                    return {from_sq, to_sq,
                            b_is_capture ? move_type_t::capture_promote_knight
                                         : move_type_t::promote_knight};
                default:
                    assert(false && "invalid promotion char");
                    return {};
            }
        }

        // En passant: diagonal pawn move onto an empty square
        const bool is_diagonal = (uci[0] != uci[2]);  // file changed
        if (is_diagonal && to_pc == piece_t::none)
        {
            return {from_sq, to_sq, move_type_t::en_passent};
        }

        // Double push: rank difference of 2
        const int rank_diff = std::abs(uci[1] - uci[3]);
        if (rank_diff == 2)
        {
            return {from_sq, to_sq, move_type_t::pawn_double};
        }
    }

    if (to_pc != piece_t::none)
    {
        return {from_sq, to_sq, move_type_t::capture};
    }
    return {from_sq, to_sq, move_type_t::quiet};
}

auto Engine::parse_run(const string& message) -> bool
{
    const auto tokens = split_into_tokens(message);

    auto parse_int = [](const string& tok) -> optional<int>
    {
        try
        {
            return stoi(tok);
        }
        catch (const exception& e)
        {
            println("Failed to convert [{}] into integer", tok);
            return nullopt;
        }
    };

    if (tokens.at(1) == "depth")
    {
        const auto depth = parse_int(tokens.at(2));
        if (!depth)
        {
            return false;
        }
        run(*depth);
        return true;
    }
    if (tokens.at(1) == "movetime")
    {
        const auto num_ms = parse_int(tokens.at(2));
        if (!num_ms)
        {
            return false;
        }
        run(chrono::milliseconds{*num_ms});
        return true;
    }
    if (tokens.at(1) == "infinite")
    {
        run();
        return true;
    }

    // --- clock-based time control: wtime/btime/winc/binc/movestogo ---
    optional<int> wtime;
    optional<int> btime;
    optional<int> winc;
    optional<int> binc;
    optional<int> movestogo;
    for (const auto& pair : tokens | ranges::views::drop(1) | ranges::views::chunk(2))
    {
        const auto& token = pair[0];
        const auto& value = pair[1];
        if (token == "wtime")
        {
            wtime = parse_int(value);
        }
        else if (token == "btime")
        {
            btime = parse_int(value);
        }
        else if (token == "winc")
        {
            winc = parse_int(value);
        }
        else if (token == "binc")
        {
            binc = parse_int(value);
        }
        else if (token == "movestogo")
        {
            movestogo = parse_int(value);
        }
    }

    if (wtime || btime)
    {
        const bool white_to_move = m_board.side_to_move() == side_t::white;
        const int my_time = (white_to_move ? wtime : btime).value_or(0);
        const int my_inc = (white_to_move ? winc : binc).value_or(0);

        // Very basic time management: budget a fraction of remaining time per move.
        // Assume ~30 moves left if movestogo wasn't given (sudden death).
        const int moves_left = movestogo.value_or(default_moves_left);
        int allocated_ms = (my_time / moves_left) + my_inc;

        // Never allocate more than what's left, and leave a small safety buffer.
        allocated_ms = std::min(allocated_ms, my_time - buffer_ms);
        allocated_ms = std::max(allocated_ms, min_time_ms);

        run(chrono::milliseconds{allocated_ms});
        return true;
    }

    run();
    return true;
}

void Engine::uci_loop()
{
    string message;
    m_b_uci_mode = true;
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    for (;;)
    {
        getline(cin, message);
        if (message.empty())
        {
            continue;
        }

        DEBUG_LOG("Received [{}]", message);
        if (message == "uci")
        {
            println("id name ElwellBot\nid author CElwell\nuciok");
            continue;
        }
        if (message == "isready")
        {
            println("readyok");
            continue;
        }
        if (message.starts_with("position"))
        {
            bool b_success = parse_and_set_position(message);
            if (!b_success)
            {
                LOG("Error while setting position");
            }
            continue;
        }
        if (message.starts_with("go"))
        {
            bool b_success = parse_run(message);
            if (!b_success)
            {
                LOG("Error while starting search. Ceasing search");
                m_stop_time = chrono::steady_clock::now();
            }
        }
        if (message.starts_with("stop"))
        {
            m_b_stop.store(true, memory_order_relaxed);
            m_stop_cv.notify_all();
            if (m_search_thread.joinable())
            {
                m_search_thread.join();
            }
            if (m_timer_thread.joinable())
            {
                m_timer_thread.join();
            }
        }
        if (message == "quit")
        {
            m_b_stop.store(true, memory_order_relaxed);
            m_stop_cv.notify_all();
            if (m_search_thread.joinable())
            {
                m_search_thread.join();
            }
            if (m_timer_thread.joinable())
            {
                m_timer_thread.join();
            }
            break;
        }
    }
}

Engine::~Engine()
{
    m_b_stop.store(true, memory_order_relaxed);
    if (m_search_thread.joinable())
    {
        m_search_thread.join();
    }
    if (m_timer_thread.joinable())
    {
        m_timer_thread.join();
    }
}
