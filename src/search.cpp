#include "search.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <future>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <print>
#include <thread>
#include <utility>

#include "bitboard.h"
#include "bitboard_constants.h"
#include "engine.h"
#include "eval.h"
#include "move.h"
#include "move_gen.h"
#include "ttable.h"
#include "window.h"

using namespace std;

namespace
{
// primary template declaration
template <side_t Side>
auto mate_eval(const MoveGen& move_gen, int8_t ply) noexcept -> int16_t;

constexpr int8_t max_quiescence_depth = 5;
}  // namespace

auto Engine::run(int depth) -> future<void>
{
    if (m_search_thread.joinable())
    {
        m_search_thread.join();
    }
    if (m_timer_thread.joinable())
    {
        m_timer_thread.join();
    }

    promise<void> promise;
    future<void> future = promise.get_future();

    m_b_stop = false;
    m_stop_time = chrono::steady_clock::now() + max_search_time;
    m_search_thread = thread(
        [this, depth, promise = std::move(promise)]() mutable -> void
        {
            if (m_board.side_to_move() == side_t::white)
            {
                search_async<side_t::white>(depth);
            }
            else
            {
                search_async<side_t::black>(depth);
            }
            promise.set_value();
        });
    return future;
}

auto Engine::run(chrono::milliseconds duration) -> future<void>
{
    if (m_search_thread.joinable())
    {
        m_search_thread.join();
    }
    if (m_timer_thread.joinable())
    {
        m_timer_thread.join();
    }

    promise<void> promise;
    future<void> future = promise.get_future();

    m_b_stop = false;
    m_stop_time = chrono::steady_clock::now() + duration;
    m_search_thread = thread(
        [this, promise = std::move(promise)]() mutable -> void
        {
            if (m_board.side_to_move() == side_t::white)
            {
                search_async<side_t::white>();
            }
            else
            {
                search_async<side_t::black>();
            }
            promise.set_value();
        });
    m_timer_thread = thread(
        [this]() -> void
        {
            unique_lock<mutex> lock(m_stop_cv_lock);
            m_stop_cv.wait_until(lock, m_stop_time,
                                 [this]() -> bool { return m_b_stop.load(memory_order_relaxed); });
            m_b_stop.store(true, memory_order_relaxed);
        });
    return future;
}

template <side_t Side>
void Engine::search_async()
{
    const lock_guard<std::mutex> lock(m_search_lock);
    int8_t depth_completed = 0;
    PVTable pv_completed = {};
    auto state = search_state{
        .board = m_board, .b_stop = &m_b_stop, .pv = {}, .history = m_history, .tt = {}};
    int eval_completed = 0;
    AspirationWindow window;
    while (!state.b_stop->load(memory_order_relaxed))
    {
        const auto [depth, alpha, beta] = window.next_window();
        const int16_t eval = search<Side>(
            search_args{.depth = depth, .ply = 0, .alpha = alpha, .beta = beta}, state);
        const bool b_eval_in_window = window.report_result(eval);

        if (!state.b_stop->load(memory_order_relaxed) && b_eval_in_window)
        {
            depth_completed = depth;
            eval_completed = eval;
            swap(pv_completed, state.pv);
            DEBUG_LOG("Completed depth: {}", depth_completed);
        }
    }
    convert_pv(pv_completed);
    m_uci = move_to_uci(pv_completed.best_move());
    m_algebraic = move_to_algebraic(pv_completed.best_move());

    if (m_b_uci_mode)
    {
        LOG("{}, {}, {}, {}", depth_completed, m_uci, static_cast<float>(eval_completed) / 100.0F,
            Side == side_t::white ? "White" : "Black");
        println("bestmove {}", m_uci);
    }
}

template <side_t Side>
void Engine::search_async(int depth)
{
    const lock_guard<std::mutex> lock(m_search_lock);
    auto state = search_state{
        .board = m_board, .b_stop = &m_b_stop, .pv = {}, .history = m_history, .tt = {}};

    // No use of this eval
    search<Side>(search_args{.depth = static_cast<int8_t>(depth),
                             .ply = 0,
                             .alpha = AspirationWindow::alpha_init,
                             .beta = AspirationWindow::beta_init},
                 state);

    // stop being true on a depth search means it was interrupted, discard result
    if (state.b_stop->load(memory_order_relaxed))
    {
        DEBUG_LOG("Discarding result of depth search");
        if (m_b_uci_mode)
        {
            println("bestmove ");
        }
    }
    else
    {
        m_uci = move_to_uci(state.pv.best_move());
        convert_pv(state.pv);
        if (m_b_uci_mode)
        {
            println("bestmove {}", m_uci);
        }
    }
}

template <side_t Side>
auto Engine::search(search_args args, search_state& state) -> int16_t
{
    auto& [board, b_stop, pv, history, tt] = state;
    const int16_t original_alpha = args.alpha;
    // instantly stop searching and cleanup
    if (b_stop->load(memory_order_relaxed))
    {
        return 0;
    }

    pv.clear(args.ply);

    const auto probe = tt.probe(board.hash().get(), args);
    if (probe.result == tt_probe_result::eval)
    {
        int16_t eval = probe.node->eval;
        if (is_mate_eval(eval))
        {
            eval += (eval > 0) ? static_cast<int16_t>(-args.ply) : static_cast<int16_t>(args.ply);
        }
        return eval;
    }

    // if end of iteration, return evaluation of board
    if (args.depth == 0)
    {
        return quiescence<Side>(args, state);
    }

    Move best_move = {};
    bool b_found_a_move = false;
    int16_t best_eval = numeric_limits<int16_t>::min();

    auto move_gen = MoveGen(board);

    if (probe.result == tt_probe_result::move)
    {
        move_gen.gen<Side>(probe.node->best_move);
    }
    else
    {
        move_gen.gen<Side>();
    }

    for (const auto& move : move_gen)
    {
        const bool b_is_irreversible =
            board.piece_at(move.from()) == piece::pawn<Side> || (move.type() != move_type_t::quiet);
        const inv_move inverse = board.apply_move<Side>(move);

        // check if move leaves king in check
        if (move_gen.is_king_in_check<Side>())
        {
            board.undo_move<Side>(move, inverse);
            continue;
        }
        int16_t eval = 0;
        if (history.is_threefold(board.hash()))
        {
            // draw
            eval = 0;
        }
        else
        {
            if (b_is_irreversible)
            {
                history.push_irreversible(board.hash());
            }
            else
            {
                history.push_back(board.hash());
            }
            eval = -search<~Side>(args.next(), state);
            history.pop_back();
        }

        board.undo_move<Side>(move, inverse);

        if (eval > best_eval)
        {
            best_eval = eval;
            best_move = move;
            b_found_a_move = true;
            pv.update(args.ply, best_move);
        }
        args.alpha = max(args.alpha, best_eval);
        if (args.alpha >= args.beta)
        {
            break;
        }
    }

    // If no moves, must be either stalemate or checkmate
    if (!b_found_a_move)
    {
        best_eval = mate_eval<Side>(move_gen, args.ply);
    }

    tt.store(tt_node{.key = board.hash().get(),
                     .best_move = best_move,
                     .eval = best_eval,
                     .depth = args.depth,
                     .flag = {}},
             original_alpha, args.beta, args.ply);

    return best_eval;
}

template <side_t Side>
auto Engine::quiescence(search_args args, search_state& state) -> int16_t
{
    auto& [board, b_stop, pv, history, tt] = state;
    const int16_t original_alpha = args.alpha;
    // instantly stop searching and cleanup
    if (b_stop->load(memory_order_relaxed))
    {
        return 0;
    }

    pv.clear(args.ply);

    const auto probe = tt.probe(board.hash().get(), args);
    if (probe.result == tt_probe_result::eval)
    {
        int16_t eval = probe.node->eval;
        if (is_mate_eval(eval))
        {
            eval += (eval > 0) ? static_cast<int16_t>(-args.ply) : static_cast<int16_t>(args.ply);
        }
        return eval;
    }

    auto move_gen = MoveGen(board);
    const bool b_moving_side_in_check = move_gen.is_king_in_check<Side>();

    int16_t best_eval = numeric_limits<int16_t>::min();
    if (!b_moving_side_in_check)
    {
        int16_t stand_pat = evaluate<Side>(board);
        // max quiescence depth
        if (args.depth < -max_quiescence_depth)
        {
            return stand_pat;
        }

        // If stand pat better then beta, this line will
        // never be reached anyways
        if (stand_pat >= args.beta)
        {
            return stand_pat;
        }
        best_eval = stand_pat;
        args.alpha = max(args.alpha, stand_pat);
    }

    Move best_move = {};
    bool b_found_a_move = false;

    if (probe.result == tt_probe_result::move)
    {
        move_gen.gen<Side>(probe.node->best_move);
    }
    else
    {
        move_gen.gen<Side>();
    }

    for (const auto& move : move_gen)
    {
        const move_type_t type = move.type();
        const bool b_is_interesting_move =
            type != move_type_t::quiet && type != move_type_t::castle_kingside &&
            type != move_type_t::castle_queenside && type != move_type_t::pawn_double;
        if (!b_moving_side_in_check && !b_is_interesting_move)
        {
            continue;
        }
        const bool b_is_irreversible =
            board.piece_at(move.from()) == piece::pawn<Side> || (move.type() != move_type_t::quiet);
        const inv_move inverse = board.apply_move<Side>(move);

        // check if move leaves king in check
        if (move_gen.is_king_in_check<Side>())
        {
            board.undo_move<Side>(move, inverse);
            continue;
        }
        int16_t eval = 0;
        if (history.is_threefold(board.hash()))
        {
            // draw
            eval = 0;
        }
        else
        {
            if (b_is_irreversible)
            {
                history.push_irreversible(board.hash());
            }
            else
            {
                history.push_back(board.hash());
            }
            eval = -quiescence<~Side>(args.next(), state);
            history.pop_back();
        }

        board.undo_move<Side>(move, inverse);

        if (eval > best_eval)
        {
            best_eval = eval;
            best_move = move;
            b_found_a_move = true;
            pv.update(args.ply, best_move);
        }
        args.alpha = max(args.alpha, best_eval);
        if (args.alpha >= args.beta)
        {
            break;
        }
    }

    // If no moves and in check, must be either stalemate or checkmate
    if (!b_found_a_move && b_moving_side_in_check)
    {
        best_eval = mate_eval<Side>(move_gen, args.ply);
    }
    // If no moves but not in check, then position is terminal for quiescence
    else if (!b_found_a_move && !b_moving_side_in_check)
    {
        best_eval = evaluate<Side>(board);
    }

    tt.store(tt_node{.key = board.hash().get(),
                     .best_move = best_move,
                     .eval = best_eval,
                     .depth = args.depth,
                     .flag = {}},
             original_alpha, args.beta, args.ply);

    return best_eval;
}

template void Engine::search_async<side_t::white>();
template void Engine::search_async<side_t::black>();
template void Engine::search_async<side_t::white>(int depth);
template void Engine::search_async<side_t::black>(int depth);
template auto Engine::search<side_t::white>(search_args args, search_state& state) -> int16_t;
template auto Engine::search<side_t::black>(search_args args, search_state& state) -> int16_t;

namespace
{
// primary template declaration
template <side_t Side>
auto mate_eval(const MoveGen& move_gen, int8_t ply) noexcept -> int16_t
{
    if (move_gen.is_king_in_check<Side>())
    {
        return static_cast<int16_t>(ply) - checkmate_eval;
    }
    return 0;  // stalemate
}
}  // namespace
