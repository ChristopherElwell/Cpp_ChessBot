#include "engine.h"

#include <algorithm>
#include <atomic>
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
        m_pv_uci.push_back(move_to_uci(mov, board));
        board.apply_move(mov);
    }
}

auto Engine::move_to_uci(const Move& mov, const BitBoard& board) -> string
{
    string out;
    uint64_t starting_sq = 0;
    uint64_t ending_sq = 0;

    switch (mov.type)
    {
        case move_type_t::quiet:
        case move_type_t::capture:
        case move_type_t::castle_kingside:
        case move_type_t::castle_queenside:
            starting_sq = mov.mov1 & board[mov.pc1];
            ending_sq = mov.mov1 & ~board[mov.pc1];
            out += square_coords.at(countr_zero(starting_sq));
            out += square_coords.at(countr_zero(ending_sq));
            return out;
        case move_type_t::promote:
            starting_sq = mov.mov1 & board[mov.pc1];
            ending_sq = mov.mov2;
            out += square_coords.at(countr_zero(starting_sq));
            out += square_coords.at(countr_zero(ending_sq));
            out += lower_case_piece_chars.at(static_cast<size_t>(mov.pc2) % 6);
            return out;
        case move_type_t::capture_promote:
            starting_sq = mov.mov1 & board[mov.pc1];
            ending_sq = mov.mov3;
            out += square_coords.at(countr_zero(starting_sq));
            out += square_coords.at(countr_zero(ending_sq));
            out += lower_case_piece_chars.at(static_cast<size_t>(mov.pc3) % 6);
            return out;
        case move_type_t::moves_termination:
            return "MOVES TERMINATED";
        default:
            return "UNKNOWN";
    }
    return out;
}

auto Engine::move_to_algebraic(const Move& move, BitBoard& board) -> string
{
    const uint64_t to_pos = move.mov1 & ~board[move.pc1];
    const uint64_t from_pos = move.mov1 & ~to_pos;
    string out;
    if (move.pc1 == piece_t::white_pawn || move.pc1 == piece_t::black_pawn)
    {
        switch (move.type)
        {
            case move_type_t::quiet:
                out = square_coords.at(countr_zero(to_pos));
                break;
            case move_type_t::capture:
                out = format("{}x{}", square_coords.at(countr_zero(from_pos))[0],
                             square_coords.at(countr_zero(to_pos)));
                break;
            case move_type_t::promote:
                out = format("{}{}", square_coords.at(countr_zero(move.mov2)),
                             piece_chars.at(static_cast<int>(move.pc2) % 6));
                break;
            case move_type_t::capture_promote:
                out = format("{}x{}={}", square_coords.at(countr_zero(from_pos))[0],
                             square_coords.at(countr_zero(move.mov2)),
                             piece_chars.at(static_cast<int>(move.pc3) % 6));
                break;
            case move_type_t::castle_kingside:
            case move_type_t::castle_queenside:
            default:
                return "Unknown";
                break;
        }
    }
    else
    {
        switch (move.type)
        {
            case move_type_t::quiet:
            {
                const char piece_char = piece_chars.at(static_cast<int>(move.pc1) % 6);
                out = format("{}{}", piece_char, square_coords.at(countr_zero(to_pos)));
                break;
            }
            case move_type_t::capture:
            {
                const char piece_char = piece_chars.at(static_cast<int>(move.pc1) % 6);
                out = format("{}x{}", piece_char, square_coords.at(countr_zero(to_pos)));
                break;
            }
            case move_type_t::castle_kingside:
                out = "O-O";
                break;
            case move_type_t::castle_queenside:
                out = "O-O-O";
                break;
            case move_type_t::capture_promote:
            default:
                return "Unknown";
        }
    }
    auto move_gen = MoveGen(board);
    board.apply_move(move);
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
    }

    return true;
}

auto Engine::uci_to_move(const string& uci, BitBoard& board) -> Move
{
    const uint64_t start_sq = 1ULL << square_coords_to_index.at(uci.substr(0, 2));
    const uint64_t end_sq = 1ULL << square_coords_to_index.at(uci.substr(2, 2));

    piece_t start_pc = piece_t::piece_count;
    piece_t end_pc = piece_t::piece_count;
    for (const auto piece : piece_range::all())
    {
        if (board[piece] & start_sq)
        {
            start_pc = piece;
        }
        if (board[piece] & end_sq)
        {
            end_pc = piece;
        }
    }

    assert(start_pc != piece_t::piece_count && "no piece on source square");

    // --- Castling ---
    if (start_pc == piece_t::white_king)
    {
        const uint64_t info_xor =
            (castling::white_kingside_right | castling::white_queenside_right) &
            board[piece_t::info];
        if ((start_sq | end_sq) == castling::white_kingside_king_move)
        {
            assert(end_pc == piece_t::piece_count);
            return Move::castle_kingside(piece_t::white_king, castling::white_kingside_king_move,
                                         piece_t::white_rook, castling::white_kingside_rook_move,
                                         info_xor, board[piece_t::info]);
        }
        if ((start_sq | end_sq) == castling::white_queenside_king_move)
        {
            assert(end_pc == piece_t::piece_count);
            return Move::castle_queenside(piece_t::white_queen, castling::white_queenside_king_move,
                                          piece_t::white_rook, castling::white_queenside_rook_move,
                                          info_xor, board[piece_t::info]);
        }
    }
    else if (start_pc == piece_t::black_king)
    {
        const uint64_t info_xor =
            (castling::black_kingside_right | castling::black_queenside_right) &
            board[piece_t::info];
        if ((start_sq | end_sq) == castling::black_kingside_king_move)
        {
            assert(end_pc == piece_t::piece_count);
            return Move::castle_kingside(piece_t::black_king, castling::black_kingside_king_move,
                                         piece_t::black_rook, castling::black_kingside_rook_move,
                                         info_xor, board[piece_t::info]);
        }
        if ((start_sq | end_sq) == castling::black_queenside_king_move)
        {
            assert(end_pc == piece_t::piece_count);
            return Move::castle_queenside(piece_t::black_queen, castling::black_queenside_king_move,
                                          piece_t::black_rook, castling::black_queenside_rook_move,
                                          info_xor, board[piece_t::info]);
        }
    }

    uint64_t info_xor = 0;
    if (start_pc == piece_t::white_rook)
    {
        info_xor |= start_sq & board[piece_t::info] &
                    (castling::white_kingside_right | castling::white_queenside_right);
    }
    if (start_pc == piece_t::black_rook)
    {
        info_xor |= start_sq & board[piece_t::info] &
                    (castling::black_kingside_right | castling::black_queenside_right);
    }
    if (end_pc == piece_t::white_rook)
    {
        info_xor |= end_sq & board[piece_t::info] &
                    (castling::white_kingside_right | castling::white_queenside_right);
    }
    if (end_pc == piece_t::black_rook)
    {
        info_xor |= end_sq & board[piece_t::info] &
                    (castling::black_kingside_right | castling::black_queenside_right);
    }
    if (start_pc == piece_t::white_king)
    {
        info_xor |= board[piece_t::info] &
                    (castling::white_kingside_right | castling::white_queenside_right);
    }
    if (start_pc == piece_t::black_king)
    {
        info_xor |= board[piece_t::info] &
                    (castling::black_kingside_right | castling::black_queenside_right);
    }

    // --- Pawn moves: promotion, en passant, double push ---
    if (start_pc == piece_t::white_pawn || start_pc == piece_t::black_pawn)
    {
        const bool is_white = start_pc == piece_t::white_pawn;

        // Promotion: 5th char present, e.g. "e7e8q"
        if (uci.size() == 5)
        {
            piece_t promo = piece_t::piece_count;
            switch (uci.at(4))
            {
                case 'q':
                    promo = is_white ? piece_t::white_queen : piece_t::black_queen;
                    break;
                case 'r':
                    promo = is_white ? piece_t::white_rook : piece_t::black_rook;
                    break;
                case 'b':
                    promo = is_white ? piece_t::white_bishop : piece_t::black_bishop;
                    break;
                case 'n':
                    promo = is_white ? piece_t::white_knight : piece_t::black_knight;
                    break;
                default:
                    assert(false && "invalid promotion char");
                    promo = piece_t::piece_count;
            }
            if (end_pc == piece_t::piece_count)
            {
                return Move::promote(start_pc, start_sq, promo, end_sq, info_xor,
                                     board[piece_t::info]);
            }
            return Move::promote_capture(start_pc, start_sq, end_pc, end_sq, promo, end_sq,
                                         info_xor, board[piece_t::info]);
        }

        // En passant: diagonal pawn move onto an empty square
        const bool is_diagonal = (uci[0] != uci[2]);  // file changed
        if (is_diagonal && end_pc == piece_t::piece_count)
        {
            return Move::capture(start_pc, start_sq,
                                 is_white ? piece_t::black_pawn : piece_t::white_pawn,
                                 is_white ? end_sq >> 8 : end_sq << 8, 0, board[piece_t::info]);
        }

        // Double push: rank difference of 2
        const int rank_diff = std::abs(uci[1] - uci[3]);
        if (rank_diff == 2)
        {
            return Move::quiet(start_pc, start_sq | end_sq, is_white ? start_sq << 8 : end_sq >> 8,
                               board[piece_t::info]);
        }
    }

    if (end_pc != piece_t::piece_count)
    {
        return Move::capture(start_pc, start_sq | end_sq, end_pc, end_sq, info_xor,
                             board[piece_t::info]);
    }
    return Move::quiet(start_pc, start_sq | end_sq, info_xor, board[piece_t::info]);
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
                m_stop_time = chrono::high_resolution_clock::now();
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
