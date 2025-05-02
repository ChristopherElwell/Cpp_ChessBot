#include "search.h"
#include "bitboard.h"
#include "move_gen.h"
#include "helpers.h"
#include "eval.h"
#include <cstdint>
#include <assert.h>
#include <algorithm>

SearchRes* search(BitBoard& bb, MoveGen& mg, int iter, int16_t alpha, int16_t beta){    
    SearchRes* this_result = new SearchRes();
    
    // if end of iteration, return evaluation of board
    if (!iter){
        this_result->best_eval = evaluate(bb);
        return this_result;
    }
    
    // initialize move array large enough for most possible moves in a chess position (219) with head room (240) to account for varying piece promotions
    std::array<Move,MOVES_ARRAY_LENGTH> movs;
    int best_move_idx = -1;

    if (bb[Piece::INFO] & TURN_BIT){ // white
        this_result->best_eval = INT16_MIN;
        mg.get_white_moves(movs,bb);
        
        for (int idx = 0; idx < MOVES_ARRAY_LENGTH; idx++){
            Move& m = movs[idx];
            if (m.type == movType::BOOK_END) break;
            bb.apply_move(m);
            
            assert((bb[Piece::INFO] & TURN_BIT) == 0);
            
            // check if move leaves white king in check
            if (bb[Piece::WHITE_KING] & mg.get_black_attackers(bb)){
                bb.apply_move(m);
                continue;
            }
            
            SearchRes* child_result = search(bb, mg, iter - 1, alpha, beta);
            bb.apply_move(m);
            
            if (child_result->best_eval > this_result->best_eval){
                helpers::free_search_result(this_result->best_result); // free old best move
                this_result->best_result = child_result; // set new best move to this one
                this_result->best_eval = child_result->best_eval; // set new best eval to this one
                best_move_idx = idx; // set new best move ptr to this one
                if (this_result->best_eval >= beta){
                    break;
                } 
                alpha = std::max(child_result->best_eval,alpha); // update alpha
            }             

            // alpha-beta pruning
            // if this move was not chosen, free memory
            if (this_result -> best_result != child_result){
                helpers::free_search_result(child_result);
            }
        }
        // copy best move to search return
        if (best_move_idx != -1){
            this_result->best_move = Move::copy(movs[best_move_idx]);
        } else { // if no valid move found we have either a checkmate or a stalemate
            // test if king is in check to determine checkmate vs stalemate, apply checkmate eval function to incentivise checkmates as soon as possible (prevents playing wiht the opponent forever)
            this_result->best_eval = (bb[Piece::WHITE_KING] & mg.get_black_attackers(bb)) ? -(CHECKMATE_EVAL - EARLY_CHECKMATE_INCENTIVE / (iter + 1)) : 0;
        }
    } else { // black
        this_result->best_eval = INT16_MAX;
        mg.get_black_moves(movs,bb);
        
        for (int idx = 0; idx < MOVES_ARRAY_LENGTH; idx++){
            Move& m = movs[idx];
            if (m.type == movType::BOOK_END) break;
            bb.apply_move(m);
            
            assert(bb[Piece::INFO] & TURN_BIT);
            
            // check if move leaves white king in check
            if (bb[Piece::BLACK_KING] & mg.get_white_attackers(bb)){
                bb.apply_move(m);
                continue;
            }
            
            SearchRes* child_result = search(bb, mg, iter - 1, alpha, beta);
            bb.apply_move(m);

            if (child_result->best_eval < this_result->best_eval){
                helpers::free_search_result(this_result->best_result); // free old best move
                this_result->best_result = child_result; // set new best move to this one
                this_result->best_eval = child_result->best_eval; // set new best eval to this one
                best_move_idx = idx; // set new best move ptr to this one
                if (this_result->best_eval <= alpha){
                    break;
                } 
                beta = std::min(child_result->best_eval,beta); // update beta
            }             

            // alpha-beta pruning
            // if this move was not chosen, free memory
            if (this_result -> best_result != child_result){
                helpers::free_search_result(child_result);
            }
        }
        // copy best move to search return
        if (best_move_idx != -1){
            this_result->best_move = Move::copy(movs[best_move_idx]);
        } else { // if no valid move found we have either a checkmate or a stalemate
            // test if king is in check to determine checkmate vs stalemate, apply checkmate eval function to incentivise checkmates as soon as possible (prevents playing wiht the opponent forever)
            this_result->best_eval = (bb[Piece::BLACK_KING] & mg.get_white_attackers(bb)) ? (CHECKMATE_EVAL - EARLY_CHECKMATE_INCENTIVE / (iter + 1)) : 0;
        }
    }
    return this_result;
}

