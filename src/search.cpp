#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <print>
#include <thread>
#include <utility>

#include "bitboard.h"
#include "engine.h"
#include "eval.h"
#include "move.h"
#include "move_gen.h"

using namespace std;

/******************************************************************/

template <side_t Side>
constexpr int init_eval = 0;  // default (optional)

template <>
constexpr int init_eval<side_t::white> = numeric_limits<int>::min();

template <>
constexpr int init_eval<side_t::black> = numeric_limits<int>::max();

/******************************************************************/

namespace
{
// primary template declaration
template <side_t Side>
constexpr auto mate_eval(const MoveGen& move_gen, int ply) noexcept -> int;

// explicit specializations
template <>
constexpr auto mate_eval<side_t::white>(const MoveGen& move_gen, int ply) noexcept -> int
{
    if (move_gen.is_king_in_check<side_t::white>())
    {
        return -checkmate_eval + ply;  // checkmate for black
    }
    return 0;  // stalemate
}

template <>
constexpr auto mate_eval<side_t::black>(const MoveGen& move_gen, int ply) noexcept -> int
{
    if (move_gen.is_king_in_check<side_t::black>())
    {
        return checkmate_eval - ply;  // checkmate for white
    }
    return 0;  // stalemate
}
}  // namespace

void Engine::run(int depth)
{
    if (m_search_thread.joinable())
    {
        m_search_thread.join();
    }
    if (m_timer_thread.joinable())
    {
        m_timer_thread.join();
    }

    m_b_stop = false;
    m_stop_time = chrono::high_resolution_clock::now() + max_search_time;
    m_search_thread = thread(
        [this, depth]() -> void
        {
            if (m_board.side_to_move() == side_t::white)
            {
                search_async<side_t::white>(depth);
            }
            else
            {
                search_async<side_t::black>(depth);
            }
        });
}

void Engine::run(chrono::milliseconds duration)
{
    if (m_search_thread.joinable())
    {
        m_search_thread.join();
    }
    if (m_timer_thread.joinable())
    {
        m_timer_thread.join();
    }

    m_b_stop = false;
    m_stop_time = chrono::high_resolution_clock::now() + duration;
    m_search_thread = thread(
        [this]() -> void
        {
            if (m_board.side_to_move() == side_t::white)
            {
                search_async<side_t::white>();
            }
            else
            {
                search_async<side_t::black>();
            }
        });
    m_timer_thread = thread(
        [this]() -> void
        {
            unique_lock<mutex> lock(m_stop_cv_lock);
            m_stop_cv.wait_until(lock, m_stop_time,
                                 [this]() -> bool { return m_b_stop.load(memory_order_relaxed); });
            m_b_stop.store(true, memory_order_relaxed);
        });
}

template <side_t Side>
void Engine::search_async()
{
    const lock_guard<std::mutex> lock(m_search_lock);
    int depth_completed = 0;
    PVTable pv_completed = {};
    auto state = search_state{.board = m_board, .b_stop = &m_b_stop, .pv = {}};
    for (int depth = 1; !state.b_stop->load(memory_order_relaxed); depth++)
    {
        // No use of this eval
        search<Side>(search_args{.depth = depth, .alpha = alpha_init, .beta = beta_init, .ply = 0},
                     state);

        if (!state.b_stop->load(memory_order_relaxed))
        {
            depth_completed = depth;
            swap(pv_completed, state.pv);
        }
    }
    LOG("Completed depth: {}", depth_completed);
    m_uci = move_to_uci(pv_completed.best_move());
    m_algebraic = move_to_algebraic(state.pv.best_move());
    convert_pv(pv_completed);
    println("bestmove {}", m_uci);
}

template <side_t Side>
void Engine::search_async(int depth)
{
    const lock_guard<std::mutex> lock(m_search_lock);
    PVTable pv_completed = {};
    auto state = search_state{.board = m_board, .b_stop = &m_b_stop, .pv = {}};

    // No use of this eval
    search<Side>(search_args{.depth = depth, .alpha = alpha_init, .beta = beta_init, .ply = 0},
                 state);

    if (!state.b_stop->load(memory_order_relaxed))
    {
        swap(pv_completed, state.pv);
    }
    m_uci = move_to_uci(pv_completed.best_move());
    m_algebraic = move_to_algebraic(pv_completed.best_move());
    convert_pv(pv_completed);
    println("bestmove {}", m_uci);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
template <side_t Side>
auto Engine::search(search_args args, search_state& state) -> int
{
    auto [depth, alpha, beta, ply] = args;
    auto& [board, b_stop, pv] = state;
    // instantly stop searching and cleanup
    if (b_stop->load(memory_order_relaxed))
    {
        return 0;
    }

    pv.clear(ply);

    // if end of iteration, return evaluation of board
    if (depth == 0)
    {
        return evaluate(board);
    }

    Move best_move = {};
    bool b_found_a_move = false;
    int best_eval = init_eval<Side>;

    auto move_gen = MoveGen(board);
    move_gen.gen<Side>();
    for (const auto& move : move_gen)
    {
        board.apply_move(move);

        // check if move leaves king in check
        if (move_gen.is_king_in_check<Side>())
        {
            board.apply_move(move);
            continue;
        }

        const int eval = search<~Side>(
            search_args{.depth = depth - 1, .alpha = alpha, .beta = beta, .ply = ply + 1}, state);
        board.apply_move(move);

        if constexpr (Side == side_t::white)
        {
            if (eval > best_eval)
            {
                best_eval = eval;
                best_move = move;
                b_found_a_move = true;
                pv.update(ply, best_move);
            }
            alpha = max(alpha, best_eval);
            if (alpha >= beta)
            {
                break;
            }
        }
        else
        {  // black
            if (eval < best_eval)
            {
                best_eval = eval;
                best_move = move;
                b_found_a_move = true;
                pv.update(ply, best_move);
            }
            beta = min(beta, best_eval);
            if (beta <= alpha)
            {
                break;
            }
        }
    }

    // If no moves, must be either stalemate or checkmate
    if (!b_found_a_move)
    {
        return mate_eval<Side>(move_gen, ply);
    }

    return best_eval;
}

template void Engine::search_async<side_t::white>();
template void Engine::search_async<side_t::black>();
template void Engine::search_async<side_t::white>(int depth);
template void Engine::search_async<side_t::black>(int depth);
template auto Engine::search<side_t::white>(search_args args, search_state& state) -> int;
template auto Engine::search<side_t::black>(search_args args, search_state& state) -> int;
