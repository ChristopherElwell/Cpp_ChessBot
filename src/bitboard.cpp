#include "bitboard.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <format>
#include <string>
#include <unordered_map>

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
                m_board[static_cast<int>(piece_t::info)] |= castling::white_kingside_right;
                break;
            case 'Q':
                m_board[static_cast<int>(piece_t::info)] |= castling::white_queenside_right;
                break;
            case 'k':
                m_board[static_cast<int>(piece_t::info)] |= castling::black_kingside_right;
                break;
            case 'q':
                m_board[static_cast<int>(piece_t::info)] |= castling::black_queenside_right;
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
    if (info & castling::white_kingside_right)
    {
        rights += 'K';
    }
    if (info & castling::white_queenside_right)
    {
        rights += 'Q';
    }
    if (info & castling::black_kingside_right)
    {
        rights += 'k';
    }
    if (info & castling::black_queenside_right)
    {
        rights += 'q';
    }
    fen += rights.empty() ? "-" : rights;

    // --- en passant square ---
    fen += ' ';
    const uint64_t known_flags = turn_bit | castling::white_kingside_right |
                                 castling::white_queenside_right | castling::black_kingside_right |
                                 castling::black_queenside_right;
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

void BitBoard::apply_move(const Move& move)
{
    assert(move.type != move_type_t::moves_termination);
    m_board[static_cast<int>(move.pc1)] ^= move.mov1;
    m_board[static_cast<int>(move.pc2)] ^= move.mov2;
    m_board[static_cast<int>(move.pc3)] ^= move.mov3;

    m_board[static_cast<int>(piece_t::info)] ^= (move.info | turn_bit);

    // TODO: xor optimize
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
}

auto BitBoard::draw() const -> string
{
    const string reset = "\033[0m";
    const string light = "\033[48;5;187m";
    const string dark = "\033[48;5;108m";

    string out = "8 ";
    for (int i = 0; i < BitBoard::num_squares; i++)
    {
        out += (((i % 8) + (i / 8)) % 2 == 0 ? light : dark);
        int piece_found = 0;
        for (const auto piece : piece_range::all())
        {
            if ((operator[](piece) & (1ULL << (BitBoard::num_squares - 1 - i))) != 0)
            {
                out += piece_emojis[static_cast<int>(piece)];
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
            out += format("{}\n{} ", reset, 8 - ((i + 1) / 8));
        }
    }
    return out + reset + "\n  a b c d e f g h\n\n";
}

auto BitBoard::sq_from_name(char file, char rank) -> uint64_t
{
    return masks::ranks[rank - '1'] & masks::files['h' - file];
}
