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

void Engine::run(int depth)
{
    unique_ptr<const result_t> p_result;
    auto state = search_state{.board = m_board, .b_stop = false, .pv = {}};
    if (m_board.whites_turn())
    {
        search<side_t::white>(
            search_args{.depth = depth, .alpha = alpha_init, .beta = beta_init, .ply = 0}, state);
    }
    else
    {
        search<side_t::black>(
            search_args{.depth = depth, .alpha = alpha_init, .beta = beta_init, .ply = 0}, state);
    }
    m_uci = move_to_uci(state.pv.best_move());
    m_algebraic = move_to_algebraic(state.pv.best_move());
#ifdef DEBUG
    convert_pv(state.pv);
#endif
}

void Engine::run(chrono::seconds timeout)
{
    auto state = search_state{.board = m_board, .b_stop = false, .pv = {}};

    thread search_thread(
        [this, &state]() -> void
        {
            if (m_board.whites_turn())
            {
                search_async<side_t::white>(state);
            }
            else
            {
                search_async<side_t::black>(state);
            }
        });

    this_thread::sleep_for(timeout);
    state.b_stop.store(true, memory_order_relaxed);
    search_thread.join();
    m_uci = move_to_uci(state.pv.best_move());
    m_algebraic = move_to_algebraic(state.pv.best_move());
#ifdef DEBUG
    convert_pv(state.pv);
#endif
}

auto Engine::get_uci() -> const string& { return m_uci; }

auto Engine::get_algebraic() -> const string& { return m_algebraic; }

void Engine::load(const string& fen) { m_board = BitBoard(fen); }

void Engine::convert_pv(const PVTable& pv_table)
{
    m_pv.clear();
    BitBoard board = m_board;
    for (const auto& mov : pv_table.get_pv_at_ply(0))
    {
        m_pv.push_back(move_to_uci(mov, board));
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
            print("id name ElwellBot\nid author CElwell\nuciok");
            continue;
        }
        if (message == "isready")
        {
            print("readyok");
            continue;
        }
        if (message.starts_with("position"))
        {
            parse_and_set_position(message);
            continue;
        }
        if (message.starts_with("go"))
        {
            // parse_run(message);
            print("{}", m_uci);
        }
        if (message == "quit")
        {
            break;
        }
    }
}
