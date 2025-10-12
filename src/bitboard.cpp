#include "bitboard.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <format>
#include <string>
#include <unordered_map>

#include "data.h"
#include "move.h"

using namespace std;

const string starting_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

using namespace std;

auto BitBoard::operator[](piece_t piece) const -> uint64_t
{
    return board[static_cast<int>(piece)];
}

auto BitBoard::start_position() -> BitBoard { return {starting_pos}; }

BitBoard::BitBoard(const string &FEN)
{
    int sqr = 0;
    unordered_map<char, piece_t> piece_t_code_map = {
        {'P', piece_t::white_pawn}, {'N', piece_t::white_knight}, {'B', piece_t::white_bishop},
        {'R', piece_t::white_rook}, {'Q', piece_t::white_queen},  {'K', piece_t::white_king},
        {'p', piece_t::black_pawn}, {'n', piece_t::black_knight}, {'b', piece_t::black_bishop},
        {'r', piece_t::black_rook}, {'q', piece_t::black_queen},  {'k', piece_t::black_king},
    };
    int idx = 0;
    for (const char piece : FEN)
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
                board[static_cast<int>(piece_t_code_map[piece])] |= 1LL << (63 - sqr);
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
    if (FEN[idx] == 'w')
    {
        board[static_cast<int>(piece_t::info)] |= TURN_BIT;
    }
    idx += 2;

    for (; FEN[idx] != ' '; idx++)
    {
        switch (FEN[idx])
        {
            case 'K':
                board[static_cast<int>(piece_t::info)] |= castling::white_kingside_right;
                break;
            case 'Q':
                board[static_cast<int>(piece_t::info)] |= castling::white_queenside_right;
                break;
            case 'k':
                board[static_cast<int>(piece_t::info)] |= castling::black_kingside_right;
                break;
            case 'q':
                board[static_cast<int>(piece_t::info)] |= castling::black_queenside_right;
                break;
            default:
                break;
        }
    }

    idx++;

    if (FEN[idx] != '-')
    {
        const char file = FEN[idx++];
        const char rank = FEN[idx++];
        board[static_cast<int>(piece_t::info)] |= sq_from_name(file, rank);
    }

    board[static_cast<int>(piece_t::white_pcs)] = board[static_cast<int>(piece_t::white_pawn)] |
                                                  board[static_cast<int>(piece_t::white_bishop)] |
                                                  board[static_cast<int>(piece_t::white_knight)] |
                                                  board[static_cast<int>(piece_t::white_rook)] |
                                                  board[static_cast<int>(piece_t::white_queen)] |
                                                  board[static_cast<int>(piece_t::white_king)];
    board[static_cast<int>(piece_t::black_pcs)] = board[static_cast<int>(piece_t::black_pawn)] |
                                                  board[static_cast<int>(piece_t::black_bishop)] |
                                                  board[static_cast<int>(piece_t::black_knight)] |
                                                  board[static_cast<int>(piece_t::black_rook)] |
                                                  board[static_cast<int>(piece_t::black_queen)] |
                                                  board[static_cast<int>(piece_t::black_king)];
    board[static_cast<int>(piece_t::all_pcs)] =
        board[static_cast<int>(piece_t::white_pcs)] | board[static_cast<int>(piece_t::black_pcs)];
}

void BitBoard::apply_move(const Move &move)
{
    assert(move.type != movType::BOOK_END);
    board[static_cast<int>(move.pc1)] ^= move.mov1;
    board[static_cast<int>(move.pc2)] ^= move.mov2;
    board[static_cast<int>(move.pc3)] ^= move.mov3;

    board[static_cast<int>(piece_t::info)] ^= (move.info | TURN_BIT);

    // TODO: xor optimize
    board[static_cast<int>(piece_t::white_pcs)] = board[static_cast<int>(piece_t::white_pawn)] |
                                                  board[static_cast<int>(piece_t::white_bishop)] |
                                                  board[static_cast<int>(piece_t::white_knight)] |
                                                  board[static_cast<int>(piece_t::white_rook)] |
                                                  board[static_cast<int>(piece_t::white_queen)] |
                                                  board[static_cast<int>(piece_t::white_king)];
    board[static_cast<int>(piece_t::black_pcs)] = board[static_cast<int>(piece_t::black_pawn)] |
                                                  board[static_cast<int>(piece_t::black_bishop)] |
                                                  board[static_cast<int>(piece_t::black_knight)] |
                                                  board[static_cast<int>(piece_t::black_rook)] |
                                                  board[static_cast<int>(piece_t::black_queen)] |
                                                  board[static_cast<int>(piece_t::black_king)];
    board[static_cast<int>(piece_t::all_pcs)] =
        board[static_cast<int>(piece_t::white_pcs)] | board[static_cast<int>(piece_t::black_pcs)];
}

auto BitBoard::draw() const -> string
{
    const string reset = "\033[0m";
    const string light = "\033[48;5;187m";
    const string dark = "\033[48;5;108m";
    string out = "8 ";
    for (int i = 0; i < 64; i++)
    {
        out += ((i % 8 + i / 8) % 2 == 0 ? light : dark);
        int piece_found = 0;
        for (const auto piece : piece_range::all())
        {
            if ((operator[](piece) & (1ULL << (63 - i))) != 0)
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
        if ((i + 1) % 8 == 0 && i != 63)
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