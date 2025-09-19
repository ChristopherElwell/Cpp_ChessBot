#include "helpers.h"
#include "bitboard.h"
#include "data.h"
#include "move.h"
#include "move_gen.h"
#include "search.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace helpers {
void free_search_result(SearchRes *result) {
  if (result == nullptr) {
    return;
  }

  SearchRes *child = result->best_result;
  result->best_result = nullptr;

  free_search_result(child);
  delete result;
}

auto sq_from_name(char file, char rank) -> uint64_t {
  return masks::RANKS[rank - '1'] & masks::FILES['h' - file];
}

void print_bitboard(const uint64_t &board) {
  for (int i = 0; i < 64; i++) {
    if (i % 8 == 0) {
      cout << 8 - (i / 8) << " ";
    }
    if ((board & (1ULL << (63 - i))) != 0) {
      cout << WHITE_SQ_CHAR;
    } else {
      cout << BLACK_SQ_CHAR;
    }
    if ((i + 1) % 8 == 0) {
      cout << "\n";
    }
  }
  cout << "  a b c d e f g h\n\n";
}

void print_move_verbose(const Move &move) {
  cout << "Move Type: " << move.type << "\n";
  cout << "Primary Piece: " << move.pc1 << "\n";

  print_bitboard(move.mov1);

  switch (move.type) {
  case movType::QUIET:
  case movType::CASTLE_KINGSIDE:
  case movType::CASTLE_QUEENSIDE:
  case movType::BOOK_END:
    break;
  case movType::CAPTURE:
    cout << "Captured Piece: " << move.pc2 << "\n";
    print_bitboard(move.mov2);
    break;
  case movType::PROMOTE:
    cout << "Promotee Piece: " << move.pc2 << "\n";
    print_bitboard(move.mov2);
    break;
  case movType::CAPTURE_PROMOTE:
    cout << "Captured Piece: " << move.pc2 << "\n";
    print_bitboard(move.mov2);
    cout << "Promotee Piece: " << move.pc3 << "\n";
    print_bitboard(move.mov3);
    break;
  }

  cout << "Info:\n";
  print_bitboard(move.info);
}

auto move_to_uci(const Move &mov, const BitBoard &board) -> string {
  if (mov.type == movType::BOOK_END) {
    return "BOOKEND";
  }
  string out;
  uint64_t starting_sq = 0;
  uint64_t ending_sq = 0;
  switch (mov.type) {
  case movType::QUIET:
  case movType::CAPTURE:
  case movType::CASTLE_KINGSIDE:
  case movType::CASTLE_QUEENSIDE:
    starting_sq = mov.mov1 & board[mov.pc1];
    ending_sq = mov.mov1 & ~board[mov.pc1];
    if (__builtin_ctzll(starting_sq) >= 64 ||
        __builtin_ctzll(ending_sq) >= 64) {
      print_bitboard(mov.mov1);
      print_bitboard(board[mov.pc1]);
      print_bitboard(starting_sq);
      print_bitboard(ending_sq);
      cout << mov;
      return "";
    }
    out += data::SQUARES[__builtin_ctzll(starting_sq)];
    out += data::SQUARES[__builtin_ctzll(ending_sq)];
    return out;
  case movType::PROMOTE:
  case movType::CAPTURE_PROMOTE:
    starting_sq = mov.mov1 & board[mov.pc1];
    ending_sq = mov.mov2 & ~board[mov.pc2];

    cout << __builtin_ctzll(starting_sq) << ", " << __builtin_ctzll(ending_sq)
         << "\n";
    out += data::SQUARES[__builtin_ctzll(starting_sq)];
    out += data::SQUARES[__builtin_ctzll(ending_sq)];
    out += data::PIECE_CODES[static_cast<int>(mov.pc2)];
    return out;
  case movType::BOOK_END:
    return "BOOK END";
  default:
    return "UNKNOWN";
  }

  return out;
}

void print_principal_variation(const SearchRes *searchres, BitBoard board) {
  const SearchRes *curr = searchres;

  cout << "PV (eval = " << curr->best_eval << "): ";

  while (curr != nullptr && curr->best_result != nullptr) {
    cout << helpers::move_to_uci(curr->best_move, board) << ", ";
    board.apply_move(curr->best_move);
    curr = curr->best_result;
  }
}

auto read_csv(const string &filename) -> vector<vector<string>> {
  vector<vector<string>> result;
  ifstream file(filename);

  if (!file.is_open()) {
    cerr << "Error opening file\n";
    return result;
  }

  string line;
  while (getline(file, line)) {
    vector<string> row;
    stringstream stream(line);
    string cell;

    while (getline(stream, cell, ',')) {
      row.push_back(cell);
    }

    result.push_back(row);
  }

  return result;
}

auto move_to_algebreic(const Move &mov, BitBoard board) -> string {
  const uint64_t to_pos = mov.mov1 & ~board[mov.pc1];
  const uint64_t from_pos = mov.mov1 & ~to_pos;
  string out;
  string destination = data::SQUARES[__builtin_ctzll(to_pos)];
  string origin = data::SQUARES[__builtin_ctzll(from_pos)];
  if (mov.pc1 == Piece::WHITE_PAWN || mov.pc1 == Piece::BLACK_PAWN) {
    switch (mov.type) {
    case movType::QUIET:
      out = destination;
      break;
    case movType::CAPTURE:
      out = format("{}x{}", origin[0], destination);
      break;
    case movType::PROMOTE:
      out = format("{}{}", destination,
                   data::PIECE_CODES[static_cast<int>(mov.pc2) % 6]);
      break;
    case movType::CAPTURE_PROMOTE:
      out = format("{}{}", origin[0], data::SQUARES[__builtin_ctzll(mov.mov2)],
                   data::PIECE_CODES[static_cast<int>(mov.pc3) % 6]);
      break;
    case movType::CASTLE_KINGSIDE:
    case movType::CASTLE_QUEENSIDE:
    default:
      return "Unknown";
      break;
    }
  } else {
    switch (mov.type) {
    case movType::QUIET:
      out = string(1, data::PIECE_CODES[static_cast<int>(mov.pc1) % 6]) +
            destination;
      break;
    case movType::CAPTURE:

      out = string(1, data::PIECE_CODES[static_cast<int>(mov.pc1) % 6]) + "x" +
            destination;
      break;
    case movType::CASTLE_KINGSIDE:
      out = "O-O";
      break;
    case movType::CASTLE_QUEENSIDE:
      out = "O-O-O";
      break;
    case movType::CAPTURE_PROMOTE:
    default:
      return "Unknown";
    }
  }
  auto move_gen = MoveGen(board);
  if (board.whites_turn()) {
    board.apply_move(mov);
    return ((board[Piece::BLACK_KING] & move_gen.get_white_attackers(board)) !=
            0)
               ? out + "+"
               : out;
  }

  board.apply_move(mov);
  return ((board[Piece::WHITE_KING] & move_gen.get_black_attackers(board)) != 0)
             ? out + "+"
             : out;
}
} // namespace helpers
auto operator<<(ostream &outstrm, Piece piece) -> ostream & {
  return outstrm << data::PIECE_NAMES[static_cast<int>(piece)];
}

auto operator<<(ostream &outstrm, movType type) -> ostream & {
  switch (type) {
  case movType::QUIET:
    return outstrm << "Quiet";
  case movType::CAPTURE:
    return outstrm << "Capture";
  case movType::PROMOTE:
    return outstrm << "Promote";
  case movType::CAPTURE_PROMOTE:
    return outstrm << "Capture & Promote";
  case movType::CASTLE_KINGSIDE:
    return outstrm << "Castle Kingside";
  case movType::CASTLE_QUEENSIDE:
    return outstrm << "Castle Queenside";
  case movType::BOOK_END:
    return outstrm << "Bookend";
  }
  return outstrm << "Unknown";
}

auto operator<<(ostream &outstrm, const Move &move) -> ostream & {
  switch (move.type) {
  case movType::QUIET:
    return outstrm << "Quiet " << move.pc1 << "\n";
  case movType::CAPTURE:
    return outstrm << move.pc1 << " takes " << move.pc2 << "\n";
  case movType::PROMOTE:
    return outstrm << move.pc1 << " promotes to " << move.pc2 << "\n";
  case movType::CAPTURE_PROMOTE:
    return outstrm << move.pc1 << " takes " << move.pc2 << " and promotes to "
                   << move.pc3 << "\n";
  case movType::CASTLE_KINGSIDE:
    return outstrm << "O-O\n";
  case movType::CASTLE_QUEENSIDE:
    return outstrm << "O-O-O\n";
  case movType::BOOK_END:
    return outstrm << "Bookend\n";
  }
  return outstrm << "unknown move";
}

auto operator<<(ostream &outstrm, const BitBoard &board) -> ostream & {
  const string reset = "\033[0m";
  const string light = "\033[48;5;187m";
  const string dark = "\033[48;5;66m";
  outstrm << "8 ";
  for (int i = 0; i < 64; i++) {
    cout << ((i % 8 + i / 8) % 2 == 0 ? light : dark);
    int piece_found = 0;
    for (const auto piece : PieceRange::all()) {
      if ((board[piece] & (1ULL << (63 - i))) != 0) {
        outstrm << pc_chars[static_cast<int>(piece)] << " ";
        piece_found = 1;
        break;
      }
    }
    if (piece_found == 0) {
      outstrm << "  ";
    }
    if ((i + 1) % 8 == 0 && i != 63) {
      outstrm << reset << "\n" << 8 - ((i + 1) / 8) << " ";
    }
  }
  return outstrm << reset << "\n  a b c d e f g h\n\n";
}
