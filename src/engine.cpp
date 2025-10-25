#include "engine.h"

#include <chrono>
#include <climits>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <limits>
#include <print>
#include <string>
#include <thread>

#include "bitboard.h"
#include "move_gen.h"
#include "search.h"

using namespace std;

void Engine::run(int depth)
{
    unique_ptr<const result_t> p_result;
    atomic_bool b_stop = false;
    if (m_board.whites_turn())
    {
        tie(m_evaluation, p_result) = search<side_t::white>(
            m_board, depth, numeric_limits<int>::min(), numeric_limits<int>::max(), b_stop);
    }
    else
    {
        tie(m_evaluation, p_result) = search<side_t::black>(
            m_board, depth, numeric_limits<int>::min(), numeric_limits<int>::max(), b_stop);
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
    bool b_whites_turn = m_board.whites_turn();
    
    thread search_thread([this, &b_stop, &result](){
        if (m_board.whites_turn()){
            search_async<side_t::white>(result, m_board, b_stop);
        } else {
            search_async<side_t::black>(result, m_board, b_stop);
        }
    });

    this_thread::sleep_for(timeout);
    b_stop = true;
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
    if (move.pc1 == piece_t::white_pawn || move.pc1 == piece_t::black_pawn)
    {
        switch (move.type)
        {
            case movType::QUIET:
                out = square_coords[__builtin_ctzll(to_pos)];
                break;
            case movType::CAPTURE:
                out = format("{}x{}", square_coords[__builtin_ctzll(from_pos)][0], square_coords[__builtin_ctzll(to_pos)]);
                break;
            case movType::PROMOTE:
                out = format("{}{}", square_coords[__builtin_ctzll(move.mov2)], piece_chars[static_cast<int>(move.pc2) % 6]);
                break;
            case movType::CAPTURE_PROMOTE:
                out = format("{}{}", square_coords[__builtin_ctzll(from_pos)][0], square_coords[__builtin_ctzll(move.mov2)],
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
                out = string(1, piece_chars[static_cast<int>(move.pc1) % 6]) + square_coords[__builtin_ctzll(to_pos)];
                break;
            case movType::CAPTURE:

                out = string(1, piece_chars[static_cast<int>(move.pc1) % 6]) + "x" + square_coords[__builtin_ctzll(to_pos)];
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

auto Engine::split_into_tokens(const string& str) -> vector<string> {
    vector<string> result;
    
    auto start = str.begin();
    
    while (start != str.end()) {
        // Find opening bracket
        start = std::find(start, str.end(), '[');
        if (start == str.end()){
            break;
        } 
        
        start++;
        
        auto end = std::find(start, str.end(), ']');
        if (end == str.end()){
            break;
        } 
        
        // Extract token
        result.emplace_back(start, end);
        
        start = end + 1;
    }
    
    return result;
}

auto Engine::handle_position(const string& token) -> bool {        
    if (token == "startpos") {
        m_board = BitBoard::start_position();
        return true;
    }
    try {
        m_board = BitBoard(token);
    } catch (exception& e){
        return false;
    }
    return true;
}

auto Engine::handle_go(const std::string& type_str, const std::string& value_str) -> bool{    
    
    int value = 0;
    try {
        value = stoi(value_str);
    } catch (exception& e){
        println("Failed at stoi: [{}]", value_str);
        return false;
    }    
    if (type_str == "depth"){
        run(value);
    } else if (type_str == "time") {
        run(chrono::seconds(value));
    } else {
        return false;
    }
    return true;
}

void Engine::uci_loop() {
    string line;
    
    for (;;) {
        getline(cin, line);
        if (line.empty()) { 
            continue;
        }
        
        auto tokens = split_into_tokens(line);
        
        // Debug output - but check size first!
        cerr << "Received " << tokens.size() << " tokens: ";
        for (const auto& token : tokens) {
            cerr << "[" << token << "] ";
        }
        cerr << "\n";
        
        if (tokens.empty()) {
            continue;
        }
        
        if (tokens.at(0) == "ready"){
            cout << "ElwellBot ready" << endl;  // endl flushes automatically
        }              
        else if (tokens.at(0) == "go"){
            if (tokens.size() < 4) {
                cout << "Error: go command needs 4 tokens" << endl;
                continue;
            }
            
            if(!handle_position(tokens.at(1))){
                cout << "Failed to set position" << endl;
                continue;
            }
            if (!handle_go(tokens.at(2), tokens.at(3))){
                cout << "Failed to get best move" << endl;
                continue;
            }
            cout << "bestmove " << get_uci() << endl;
        } else {
            cout << "Did not recognize command: " << tokens.at(0) << endl;
        }
        
        cout.flush();  // Extra safety - ensure output is sent
    }
}