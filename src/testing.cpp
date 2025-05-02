#include "testing.h"
#include "move.h"
#include "move_gen.h"
#include "bitboard.h"
#include "helpers.h"
#include <array>
#include <string>
#include <cstdint>
#include <utility>

namespace tests {
    const std::array<std::pair<std::string,std::array<uint64_t,6>>,6> PERFT_DATA = {
        std::make_pair("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",std::array<uint64_t,6>{20,400,8902,197281,4865609,119060324}),
        std::make_pair("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ",std::array<uint64_t,6>{48,2039,97862,4085603,193690690,8031647685}),
        std::make_pair("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ",std::array<uint64_t,6>{14,191,2812,43238,674624,11030083}),
        std::make_pair("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",std::array<uint64_t,6>{6,264,9467,422333,15833292,706045033}),
        std::make_pair("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1 ",std::array<uint64_t,6>{6,264,9467,422333,15833292,706045033}),
        std::make_pair("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 ",std::array<uint64_t,6>{46,2079,89890,3894594,164075551,6923051137})
    };

    uint64_t perft_search(BitBoard& bb, MoveGen& mg, int iter){
        if (!iter){
            return 1;
        }
        int64_t perft = 0;
        std::array<Move,MOVES_ARRAY_LENGTH> movs;
        
        if (bb[Piece::INFO] & TURN_BIT){
            mg.get_white_moves(movs,bb);
            for (Move m : movs){
                if (m.type == movType::BOOK_END) break;
                bb.apply_move(m);
                // check if move leaves white king in check
                if (bb[Piece::WHITE_KING] & mg.get_black_attackers(bb)){
                    bb.apply_move(m);
                    continue;
                }
                perft += perft_search(bb,mg,iter - 1);
                bb.apply_move(m);
            }
        } else {
            mg.get_black_moves(movs,bb);
            for (Move m : movs){
                if (m.type == movType::BOOK_END) break;
                bb.apply_move(m);
                // check if move leaves white king in check
                if (bb[Piece::BLACK_KING] & mg.get_white_attackers(bb)){
                    bb.apply_move(m);
                    continue;
                }
                perft += perft_search(bb,mg,iter - 1);
                bb.apply_move(m);
            }
        }
        return perft;
    
    }

    uint64_t perft_search_declare(std::string FEN, int iter){
        MoveGen mg;
        BitBoard bb = BitBoard(FEN);
        int64_t total_perft = 0;
        std::array<Move,MOVES_ARRAY_LENGTH> movs;
        
        if (bb[Piece::INFO] & TURN_BIT){
            mg.get_white_moves(movs,bb);
            for (Move m : movs){
                if (m.type == movType::BOOK_END) break;
                bb.apply_move(m);
                // check if move leaves white king in check
                if (bb[Piece::WHITE_KING] & mg.get_black_attackers(bb)){
                    bb.apply_move(m);
                    continue;
                }
                uint64_t perft = perft_search(bb,mg,iter - 1);
                total_perft += perft;
                bb.apply_move(m);
                std::cout << move_to_uci(m,bb) << ": " << perft << "\n";
            }
        } else {
            mg.get_white_moves(movs,bb);
            for (Move m : movs){
                if (m.type == movType::BOOK_END) break;
                bb.apply_move(m);
                // check if move leaves white king in check
                if (bb[Piece::BLACK_KING] & mg.get_white_attackers(bb)){
                    bb.apply_move(m);
                    continue;
                }
                uint64_t perft = perft_search(bb,mg,iter - 1);
                total_perft += perft;
                bb.apply_move(m);
                std::cout << move_to_uci(m,bb) << ": " << perft;
            }
        }
        return total_perft;
    }

    void run_perft_test(int max_draft){
        MoveGen mg;
        int pftcntr = 1;
        int tests_passed = 0;
        for (auto perft_test : PERFT_DATA){
            BitBoard bb = BitBoard(perft_test.first);
            std::cout << "=======================================\nPERFT TEST " << pftcntr++ << "\n";
            int drafts_passed = 0;
            for (int i = 0; i < max_draft; i++){
                uint64_t perft = perft_search(bb,mg,i+1);
                if (perft == perft_test.second[i]){
                    std::cout<<"\tPassed, Draft: " << i+1 << 
                    " | Correct Perft: " << perft << "\n";
                    drafts_passed++;
                } else {
                    std::cout<<"\tFailed, Draft: " << i+1 << 
                    " | Correct Perft: " << perft_test.second[i] << 
                    " | This Perft: " << perft << "\n";
                }
            }
            std::cout<< "DRAFTS PASSED: " << drafts_passed << "/" << max_draft << "\n";
            std::cout<<"=======================================\n\n\n";
            if(drafts_passed==max_draft) tests_passed++;
        }
        std::cout << "TESTS PASSED: " << tests_passed << "/" << 6 << "\n";
    }

}