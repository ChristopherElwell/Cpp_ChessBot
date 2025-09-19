#include "bitboard.h"
#include "data.h"
#include "helpers.h"
#include "move.h"
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

using namespace std;

auto BitBoard::operator[](Piece piece) const -> uint64_t {
  return board[static_cast<int>(piece)];
}

BitBoard::BitBoard(const string &FEN) {
  int sqr = 0;
  unordered_map<char, Piece> Piece_code_map = {
      {'P', Piece::WHITE_PAWN},   {'N', Piece::WHITE_KNIGHT},
      {'B', Piece::WHITE_BISHOP}, {'R', Piece::WHITE_ROOK},
      {'Q', Piece::WHITE_QUEEN},  {'K', Piece::WHITE_KING},
      {'p', Piece::BLACK_PAWN},   {'n', Piece::BLACK_KNIGHT},
      {'b', Piece::BLACK_BISHOP}, {'r', Piece::BLACK_ROOK},
      {'q', Piece::BLACK_QUEEN},  {'k', Piece::BLACK_KING},
  };
  int idx = 0;
  for (const char piece : FEN) {
    switch (piece) {
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
      board[static_cast<int>(Piece_code_map[piece])] |= 1LL << (63 - sqr);
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
    if (piece == ' ') {
      break;
    }
    idx++;
  }

  idx++;
  if (FEN[idx] == 'w') {
    board[static_cast<int>(Piece::INFO)] |= TURN_BIT;
  }
  idx += 2;

  for (; FEN[idx] != ' '; idx++) {
    switch (FEN[idx]) {
    case 'K':
      board[static_cast<int>(Piece::INFO)] |= castling::WHITE_KINGSIDE_RIGHT;
      break;
    case 'Q':
      board[static_cast<int>(Piece::INFO)] |= castling::WHITE_QUEENSIDE_RIGHT;
      break;
    case 'k':
      board[static_cast<int>(Piece::INFO)] |= castling::BLACK_KINGSIDE_RIGHT;
      break;
    case 'q':
      board[static_cast<int>(Piece::INFO)] |= castling::BLACK_QUEENSIDE_RIGHT;
      break;
    default:
      break;
    }
  }

  idx++;

  if (FEN[idx] != '-') {
    const char file = FEN[idx++];
    const char rank = FEN[idx++];
    board[static_cast<int>(Piece::INFO)] |= helpers::sq_from_name(file, rank);
  }

  board[static_cast<int>(Piece::WHITE_PCS)] =
      board[static_cast<int>(Piece::WHITE_PAWN)] |
      board[static_cast<int>(Piece::WHITE_BISHOP)] |
      board[static_cast<int>(Piece::WHITE_KNIGHT)] |
      board[static_cast<int>(Piece::WHITE_ROOK)] |
      board[static_cast<int>(Piece::WHITE_QUEEN)] |
      board[static_cast<int>(Piece::WHITE_KING)];
  board[static_cast<int>(Piece::BLACK_PCS)] =
      board[static_cast<int>(Piece::BLACK_PAWN)] |
      board[static_cast<int>(Piece::BLACK_BISHOP)] |
      board[static_cast<int>(Piece::BLACK_KNIGHT)] |
      board[static_cast<int>(Piece::BLACK_ROOK)] |
      board[static_cast<int>(Piece::BLACK_QUEEN)] |
      board[static_cast<int>(Piece::BLACK_KING)];
  board[static_cast<int>(Piece::ALL_PCS)] =
      board[static_cast<int>(Piece::WHITE_PCS)] |
      board[static_cast<int>(Piece::BLACK_PCS)];
}

void BitBoard::apply_move(const Move &move) {

  board[static_cast<int>(move.pc1)] ^= move.mov1;
  board[static_cast<int>(move.pc2)] ^= move.mov2;
  board[static_cast<int>(move.pc3)] ^= move.mov3;

  board[static_cast<int>(Piece::INFO)] ^= (move.info | TURN_BIT);

  // TODO: xor optimize
  board[static_cast<int>(Piece::WHITE_PCS)] =
      board[static_cast<int>(Piece::WHITE_PAWN)] |
      board[static_cast<int>(Piece::WHITE_BISHOP)] |
      board[static_cast<int>(Piece::WHITE_KNIGHT)] |
      board[static_cast<int>(Piece::WHITE_ROOK)] |
      board[static_cast<int>(Piece::WHITE_QUEEN)] |
      board[static_cast<int>(Piece::WHITE_KING)];
  board[static_cast<int>(Piece::BLACK_PCS)] =
      board[static_cast<int>(Piece::BLACK_PAWN)] |
      board[static_cast<int>(Piece::BLACK_BISHOP)] |
      board[static_cast<int>(Piece::BLACK_KNIGHT)] |
      board[static_cast<int>(Piece::BLACK_ROOK)] |
      board[static_cast<int>(Piece::BLACK_QUEEN)] |
      board[static_cast<int>(Piece::BLACK_KING)];
  board[static_cast<int>(Piece::ALL_PCS)] =
      board[static_cast<int>(Piece::WHITE_PCS)] |
      board[static_cast<int>(Piece::BLACK_PCS)];
}