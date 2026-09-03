#include "bitboard.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <format>
#include <string>
#include <unordered_map>

#include "bitboard_constants.h"
#include "bitscan.h"
#include "data.h"
#include "move.h"

using namespace std;

const string starting_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

using namespace std;

auto BitBoard::operator[](piece_t piece) const -> uint64_t
{
    return m_board[static_cast<int>(piece)];
}

auto BitBoard::start_position() -> BitBoard { return {starting_pos}; }

BitBoard::BitBoard(const string& fen)
{
    int sqr = 0;
    int idx = 0;
    for (const char piece : fen)
    {
        switch (piece)
        {
            case 'P':
            case 'N':
            case 'B':
            case 'R':
            case 'Q':
            case 'K':
            case 'p':
            case 'n':
            case 'b':
            case 'r':
            case 'q':
            case 'k':
                m_board[static_cast<int>(piece_symbol_to_piece_t.at(piece))] |=
                    1LL << (BitBoard::num_squares - 1 - sqr);
                sqr++;
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
                sqr += piece - '0';
                break;
            case '/':
            case ' ':
            default:
                break;
        }
        if (piece == ' ')
        {
            break;
        }
        idx++;
    }

    idx++;
    if (fen[idx] == 'w')
    {
        m_board[static_cast<int>(piece_t::info)] |= turn_bit;
    }
    idx += 2;

    for (; fen[idx] != ' '; idx++)
    {
        switch (fen[idx])
        {
            case 'K':
                m_board[static_cast<int>(piece_t::info)] |= castling<side_t::white>::kingside_right;
                break;
            case 'Q':
                m_board[static_cast<int>(piece_t::info)] |=
                    castling<side_t::white>::queenside_right;
                break;
            case 'k':
                m_board[static_cast<int>(piece_t::info)] |= castling<side_t::black>::kingside_right;
                break;
            case 'q':
                m_board[static_cast<int>(piece_t::info)] |=
                    castling<side_t::black>::queenside_right;
                break;
            default:
                break;
        }
    }

    idx++;

    if (fen[idx] != '-')
    {
        const char file = fen[idx++];
        const char rank = fen[idx++];
        m_board[static_cast<int>(piece_t::info)] |= sq_from_name(file, rank);
    }

    m_board[static_cast<int>(piece_t::white_pcs)] =
        m_board[static_cast<int>(piece_t::white_pawn)] |
        m_board[static_cast<int>(piece_t::white_bishop)] |
        m_board[static_cast<int>(piece_t::white_knight)] |
        m_board[static_cast<int>(piece_t::white_rook)] |
        m_board[static_cast<int>(piece_t::white_queen)] |
        m_board[static_cast<int>(piece_t::white_king)];
    m_board[static_cast<int>(piece_t::black_pcs)] =
        m_board[static_cast<int>(piece_t::black_pawn)] |
        m_board[static_cast<int>(piece_t::black_bishop)] |
        m_board[static_cast<int>(piece_t::black_knight)] |
        m_board[static_cast<int>(piece_t::black_rook)] |
        m_board[static_cast<int>(piece_t::black_queen)] |
        m_board[static_cast<int>(piece_t::black_king)];
    m_board[static_cast<int>(piece_t::all_pcs)] = m_board[static_cast<int>(piece_t::white_pcs)] |
                                                  m_board[static_cast<int>(piece_t::black_pcs)];
    m_hash = ZobristHash(*this);
}

auto BitBoard::to_fen() const -> string
{
    // index -> char lookup, built from each piece bitboard via bit_scan
    std::array<char, BitBoard::num_squares> board_chars{};
    board_chars.fill(0);

    for (const auto piece : piece_range::all())
    {
        for (const uint64_t bit : bit_scan(m_board[static_cast<int>(piece)]))
        {
            const int index = std::countr_zero(bit);  // requires <bit>, C++20
            board_chars[index] = piece_t_to_piece_symbol.at(piece);
        }
    }

    // --- piece placement, rank 8 -> 1, file a -> h ---
    string fen;
    for (int rank = 8; rank >= 1; --rank)
    {
        int empty_count = 0;
        for (int file = 0; file <= 7; ++file)
        {
            const int index = (8 * rank) - file - 1;
            const char piece_symbol = board_chars[index];
            if (piece_symbol == 0)
            {
                ++empty_count;
                continue;
            }
            if (empty_count > 0)
            {
                fen += std::to_string(empty_count);
                empty_count = 0;
            }
            fen += piece_symbol;
        }
        if (empty_count > 0)
        {
            fen += std::to_string(empty_count);
        }
        if (rank != 1)
        {
            fen += '/';
        }
    }

    // --- side to move ---
    const uint64_t info = m_board[static_cast<int>(piece_t::info)];
    fen += ' ';
    fen += (info & turn_bit) ? 'w' : 'b';

    // --- castling rights ---
    fen += ' ';
    string rights;
    if (info & castling<side_t::white>::kingside_right)
    {
        rights += 'K';
    }
    if (info & castling<side_t::white>::queenside_right)
    {
        rights += 'Q';
    }
    if (info & castling<side_t::black>::kingside_right)
    {
        rights += 'k';
    }
    if (info & castling<side_t::black>::queenside_right)
    {
        rights += 'q';
    }
    fen += rights.empty() ? "-" : rights;

    // --- en passant square ---
    fen += ' ';
    const uint64_t known_flags = turn_bit | castling<side_t::white>::kingside_right |
                                 castling<side_t::white>::queenside_right |
                                 castling<side_t::black>::kingside_right |
                                 castling<side_t::black>::queenside_right;
    const uint64_t ep_bits = info & ~known_flags;
    if (ep_bits == 0)
    {
        fen += '-';
    }
    else
    {
        const int index = std::countr_zero(ep_bits);
        const int sqr = BitBoard::num_squares - 1 - index;  // inverse of the constructor's packing
        const int rank = 8 - (sqr / 8);
        const char file = static_cast<char>('a' + (sqr % 8));
        fen += file;
        fen += std::to_string(rank);
    }

    return fen;
}

template <side_t Side>
auto BitBoard::apply_move(const Move& move) -> inv_move
{
    const uint64_t from_mask = 1ULL << move.from();
    const uint64_t to_mask = 1ULL << move.to();
    const uint64_t mov_mask = from_mask | to_mask;
    const piece_t moving_pc = piece_at(from_mask);
    const piece_t captured_pc = piece_at(to_mask);

    const auto inverse = inv_move{.moving_pc = moving_pc,
                                  .captured_pc = captured_pc,
                                  .info = m_board.at(static_cast<size_t>(piece_t::info))};

    const uint64_t en_passent_sq = m_board[static_cast<size_t>(piece_t::info)] & ~masks::rank_1;
    m_board[static_cast<size_t>(piece_t::info)] ^= turn_bit | en_passent_sq;
    m_hash.push_info(turn_bit | en_passent_sq);
    if (moving_pc == piece::rook<Side>)
    {
        if (mov_mask & castling<Side>::kingside_rook_from &&
            m_board.at(static_cast<size_t>(piece_t::info)) & castling<Side>::kingside_right)
        {
            m_board.at(static_cast<size_t>(piece_t::info)) ^= castling<Side>::kingside_right;
            m_hash.push_info(castling<Side>::kingside_right);
        }
        if (mov_mask & castling<Side>::queenside_rook_from &&
            m_board.at(static_cast<size_t>(piece_t::info)) & castling<Side>::queenside_right)
        {
            m_board.at(static_cast<size_t>(piece_t::info)) ^= castling<Side>::queenside_right;
            m_hash.push_info(castling<Side>::queenside_right);
        }
    }
    if (moving_pc == piece::king<Side>)
    {
        if (m_board.at(static_cast<size_t>(piece_t::info)) & castling<Side>::kingside_right)
        {
            m_hash.push_info(castling<Side>::kingside_right);
            m_board.at(static_cast<size_t>(piece_t::info)) ^= castling<Side>::kingside_right;
        }
        if (m_board.at(static_cast<size_t>(piece_t::info)) & castling<Side>::queenside_right)
        {
            m_board.at(static_cast<size_t>(piece_t::info)) ^= castling<Side>::queenside_right;
            m_hash.push_info(castling<Side>::queenside_right);
        }
    }
    if (captured_pc == piece::rook<~Side>)
    {
        if (to_mask & castling<~Side>::kingside_rook_from &&
            m_board.at(static_cast<size_t>(piece_t::info)) & castling<~Side>::kingside_right)
        {
            m_board.at(static_cast<size_t>(piece_t::info)) ^= castling<~Side>::kingside_right;
            m_hash.push_info(castling<~Side>::kingside_right);
        }
        if (to_mask & castling<~Side>::queenside_rook_from &&
            m_board.at(static_cast<size_t>(piece_t::info)) & castling<~Side>::queenside_right)
        {
            m_board.at(static_cast<size_t>(piece_t::info)) ^= castling<~Side>::queenside_right;
            m_hash.push_info(castling<~Side>::queenside_right);
        }
    }

    mask_move<Side>(move.type(), inverse.moving_pc, from_mask, to_mask, inverse.captured_pc);

    return inverse;
}

auto BitBoard::apply_move(const Move& move) -> inv_move
{
    if (side_to_move() == side_t::white)
    {
        return apply_move<side_t::white>(move);
    }
    return apply_move<side_t::black>(move);
}

template <side_t Side>
void BitBoard::undo_move(const Move& move, const inv_move& inverse)
{
    const uint64_t from_mask = 1ULL << move.from();
    const uint64_t to_mask = 1ULL << move.to();

    mask_move<Side>(move.type(), inverse.moving_pc, from_mask, to_mask, inverse.captured_pc);

    m_hash.push_info(m_board.at(static_cast<size_t>(piece_t::info)) ^ inverse.info);
    m_board.at(static_cast<size_t>(piece_t::info)) = inverse.info;
}

void BitBoard::undo_move(const Move& move, const inv_move& inverse)
{
    if (side_to_move() == side_t::white)
    {
        undo_move<side_t::white>(move, inverse);
    }
    else
    {
        undo_move<side_t::black>(move, inverse);
    }
}

template <side_t Side>
auto BitBoard::mask_move(move_type_t type, piece_t moving_pc, uint64_t from_mask, uint64_t to_mask,
                         piece_t captured_pc)
{
    const uint64_t mov_mask = from_mask | to_mask;
    switch (type)
    {
        case move_type_t::quiet:
            apply_mask<Side>(moving_pc, mov_mask);
            break;
        case move_type_t::capture:
            apply_mask<Side>(moving_pc, mov_mask);
            apply_mask<~Side>(captured_pc, to_mask);
            break;
        case move_type_t::promote_queen:
        {
            apply_mask<Side>(moving_pc, from_mask);
            apply_mask<Side>(piece::queen<Side>, to_mask);
            break;
        }
        case move_type_t::promote_rook:
        {
            apply_mask<Side>(moving_pc, from_mask);
            apply_mask<Side>(piece::rook<Side>, to_mask);
            break;
        }
        case move_type_t::promote_bishop:
        {
            apply_mask<Side>(moving_pc, from_mask);
            apply_mask<Side>(piece::bishop<Side>, to_mask);
            break;
        }
        case move_type_t::promote_knight:
        {
            apply_mask<Side>(moving_pc, from_mask);
            apply_mask<Side>(piece::knight<Side>, to_mask);
            break;
        }
        case move_type_t::capture_promote_queen:
        {
            apply_mask<Side>(moving_pc, from_mask);
            apply_mask<~Side>(captured_pc, to_mask);
            apply_mask<Side>(piece::queen<Side>, to_mask);
            break;
        }
        case move_type_t::capture_promote_rook:
        {
            apply_mask<Side>(moving_pc, from_mask);
            apply_mask<~Side>(captured_pc, to_mask);
            apply_mask<Side>(piece::rook<Side>, to_mask);
            break;
        }
        case move_type_t::capture_promote_bishop:
        {
            apply_mask<Side>(moving_pc, from_mask);
            apply_mask<~Side>(captured_pc, to_mask);
            apply_mask<Side>(piece::bishop<Side>, to_mask);
            break;
        }
        case move_type_t::capture_promote_knight:
        {
            apply_mask<Side>(moving_pc, from_mask);
            apply_mask<~Side>(captured_pc, to_mask);
            apply_mask<Side>(piece::knight<Side>, to_mask);
            break;
        }
        case move_type_t::castle_kingside:
        {
            apply_mask<Side>(piece::king<Side>,
                             castling<Side>::kingside_king_from | castling<Side>::kingside_king_to);
            apply_mask<Side>(piece::rook<Side>,
                             castling<Side>::kingside_rook_from | castling<Side>::kingside_rook_to);
            break;
        }
        case move_type_t::castle_queenside:
        {
            apply_mask<Side>(piece::king<Side>, castling<Side>::queenside_king_from |
                                                    castling<Side>::queenside_king_to);
            apply_mask<Side>(piece::rook<Side>, castling<Side>::queenside_rook_from |
                                                    castling<Side>::queenside_rook_to);
            break;
        }
        case move_type_t::en_passent:
            apply_mask<Side>(moving_pc, mov_mask);
            if constexpr (Side == side_t::white)
            {
                apply_mask<~Side>(piece_t::black_pawn, to_mask >> 8);
            }
            else
            {
                apply_mask<~Side>(piece_t::white_pawn, to_mask << 8);
            }
            break;
        case move_type_t::pawn_double:
            apply_mask<Side>(moving_pc, mov_mask);
            if constexpr (Side == side_t::white)
            {
                m_board[static_cast<size_t>(piece_t::info)] ^= to_mask >> 8;
                m_hash.push_info(to_mask >> 8);
            }
            else
            {
                m_board[static_cast<size_t>(piece_t::info)] ^= to_mask << 8;
                m_hash.push_info(to_mask << 8);
            }
            break;
        case move_type_t::moves_termination:
            break;
    }
}

auto BitBoard::draw() const -> string
{
    string out = "8 ";
    for (int i = 0; i < BitBoard::num_squares; i++)
    {
        int piece_found = 0;
        for (const auto piece : piece_range::all())
        {
            if ((operator[](piece) & (1ULL << (BitBoard::num_squares - 1 - i))) != 0)
            {
                out += piece_t_to_piece_symbol.at(piece);
                out += " ";
                piece_found = 1;
                break;
            }
        }
        if (piece_found == 0)
        {
            out += "  ";
        }
        if ((i + 1) % 8 == 0 && i != BitBoard::num_squares - 1)
        {
            out += format("\n{} ", 8 - ((i + 1) / 8));
        }
    }
    return out + "\n  a b c d e f g h\n\n";
}

auto BitBoard::sq_from_name(char file, char rank) -> uint64_t
{
    return masks::ranks[rank - '1'] & masks::files['h' - file];
}

auto BitBoard::hash() -> ZobristHash { return m_hash; }

auto BitBoard::piece_at(int pos) -> piece_t { return piece_at(1ULL << pos); }

auto BitBoard::piece_at(uint64_t mask) -> piece_t
{
    for (const piece_t piece : piece_range::all())
    {
        if (mask & m_board.at(static_cast<size_t>(piece)))
        {
            return piece;
        }
    }
    return piece_t::none;
}

template <side_t Side>
void BitBoard::apply_mask(piece_t piece, uint64_t mask)
{
    m_board[static_cast<size_t>(piece)] ^= mask;
    m_board[static_cast<size_t>(piece::all<Side>)] ^= mask;
    m_board[static_cast<size_t>(piece_t::all_pcs)] ^= mask;
    m_hash.push_piece(piece, mask);
}
