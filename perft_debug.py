#!/usr/bin/env python3
"""
perft_debug.py

Bisects perft (move generation) discrepancies down to the exact position and
move where your engine disagrees with a reference implementation, instead of
you staring at one wrong number at the end of a huge tree.

How it works
------------
1. Runs `<engine> --divide <fen> <depth>`, which prints, for every legal move
   at the root, the perft count of the subtree under that move (the new
   `run_perft_divide` / `--divide` flag added to testing.cpp/main.cpp).
2. Computes the same divide independently using python-chess, a well-tested
   pure-Python chess library, as a reference oracle.
3. Compares the two move-by-move breakdowns:
     - A move that's legal but missing from your engine's list -> you're
       failing to generate a legal move.
     - A move your engine generates but that's illegal -> you're generating
       an illegal move (bad pin/check detection, wrong castling rights, etc).
     - A move present in both but with a different subtree count -> the bug
       is deeper in that subtree, so the script makes that move and recurses,
       dividing again one ply down.
4. This repeats until the discrepancy is a concrete missing/extra move at a
   specific position -- at which point you have the exact FEN and move path
   where things go wrong. Much easier than "perft(6) is off by 40000 nodes".

Setup
-----
    pip install chess

Usage
-----
    # Scan the 6 standard perft test positions (same ones as testing.cpp) for
    # the first depth that doesn't match, then bisect it:
    python perft_debug.py --engine ./chess --auto

    # Bisect a specific position/depth you already know is wrong:
    python perft_debug.py --engine ./chess \\
        --fen "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" \\
        --depth 4

Notes on performance
---------------------
python-chess is pure Python, so it's much slower than your C++ engine.
Bisection gets cheap fast once you're a few moves deep (positions shrink
quickly), but the *first* divide call -- at whatever depth first shows a
discrepancy -- costs roughly as much as a full perft to that depth in pure
Python. Depths 1-4 are fast (well under a minute); depth 5+ can take a
while, depth 6 likely too long to be worth it. --auto defaults to scanning
depths 1-4 for exactly this reason; raise --max-depth if you need to, but
expect it to slow down a lot.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys

try:
    import chess
except ImportError:
    print(
        "This script needs python-chess. Install it with:\n    pip install chess",
        file=sys.stderr,
    )
    sys.exit(1)


# Same positions and expected perft counts as `perft_tests` in testing.cpp, so
# --auto can reuse them directly without you having to pass a FEN by hand.
PERFT_TESTS: list[tuple[str, list[int]]] = [
    # ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    #  [20, 400, 8902, 197281, 4865609, 119060324]),
    # ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ",
    #  [48, 2039, 97862, 4085603, 193690690, 8031647685]),
    (
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        [14, 191, 2812, 43238, 674624, 11030083],
    ),
    (
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        [6, 264, 9467, 422333, 15833292, 706045033],
    ),
    (
        "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1",
        [6, 264, 9467, 422333, 15833292, 706045033],
    ),
    (
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        [46, 2079, 89890, 3894594, 164075551, 6923051137],
    ),
]

DIVIDE_LINE_RE = re.compile(r"^([a-h][1-8][a-h][1-8][nbrq]?)\s*:\s*(\d+)$")


def engine_divide(engine: str, fen: str, depth: int) -> dict[str, int]:
    """Run `<engine> --divide <fen> <depth>` and parse its "move: count" output."""
    proc = subprocess.run(
        [engine, "--divide", fen, str(depth)],
        capture_output=True,
        text=True,
        check=True,
    )
    counts: dict[str, int] = {}
    for line in proc.stdout.splitlines():
        match = DIVIDE_LINE_RE.match(line.strip())
        if match:
            counts[match.group(1)] = int(match.group(2))
    if not counts:
        print(
            "  Warning: got no parseable divide output from the engine.",
            file=sys.stderr,
        )
        print("  Raw stdout was:\n" + proc.stdout, file=sys.stderr)
        if proc.stderr:
            print("  Raw stderr was:\n" + proc.stderr, file=sys.stderr)
    return counts


def reference_perft(board: chess.Board, depth: int) -> int:
    if depth == 0:
        return 1
    count = 0
    for move in board.legal_moves:
        board.push(move)
        count += reference_perft(board, depth - 1)
        board.pop()
    return count


def reference_divide(board: chess.Board, depth: int) -> dict[str, int]:
    counts: dict[str, int] = {}
    for move in board.legal_moves:
        board.push(move)
        counts[move.uci()] = reference_perft(board, depth - 1)
        board.pop()
    return counts


def bisect(engine: str, fen: str, depth: int, path: list[str] | None = None) -> None:
    path = path or []
    print(f"\n--- depth {depth}, FEN: {fen} ---")
    if path:
        print(f"    reached via: {' '.join(path)}")

    engine_counts = engine_divide(engine, fen, depth)
    board = chess.Board(fen)
    ref_counts = reference_divide(board, depth)

    engine_moves, ref_moves = set(engine_counts), set(ref_counts)
    missing = ref_moves - engine_moves  # legal, but engine never generates them
    extra = engine_moves - ref_moves  # engine generates them, but illegal
    mismatched = {
        m for m in engine_moves & ref_moves if engine_counts[m] != ref_counts[m]
    }

    if missing:
        print(
            f"    MISSING (legal moves your engine never generates): {sorted(missing)}"
        )
    if extra:
        print(
            f"    EXTRA (moves your engine generates, but are illegal): {sorted(extra)}"
        )

    if missing or extra:
        print(
            "\n>>> Found it. This is a move-generation bug, not a deeper counting bug."
        )
        print(f">>> Position:  {fen}")
        print(f">>> Move path: {' '.join(path) if path else '(root position)'}")
        return

    if not mismatched:
        print(
            "    No discrepancy at this depth (engine and reference agree here -- "
            "the bug may be elsewhere, or this wasn't actually a failing depth)."
        )
        return

    print(
        "    Move counts differ (move exists for both, but its subtree count is wrong):"
    )
    for m in sorted(mismatched):
        print(f"      {m}:  engine={engine_counts[m]}  reference={ref_counts[m]}")

    next_move = sorted(mismatched)[0]
    print(f"    Descending into '{next_move}' to narrow further...")
    board.push_uci(next_move)
    bisect(engine, board.fen(), depth - 1, path + [next_move])


def find_first_failing_depth(
    engine: str, fen: str, expected: list[int], max_depth: int
) -> int | None:
    for depth in range(1, min(max_depth, len(expected)) + 1):
        counts = engine_divide(engine, fen, depth)
        total = sum(counts.values())
        want = expected[depth - 1]
        status = "OK" if total == want else "MISMATCH"
        print(f"  depth {depth}: engine={total}  expected={want}  [{status}]")
        if total != want:
            return depth
    return None


def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--engine", required=True, help="path to your compiled engine binary"
    )
    parser.add_argument("--fen", help="FEN to bisect (use together with --depth)")
    parser.add_argument(
        "--depth", type=int, help="depth known to be wrong (use together with --fen)"
    )
    parser.add_argument(
        "--auto",
        action="store_true",
        help="scan the 6 standard perft test positions for the first wrong depth, "
        "then bisect it",
    )
    parser.add_argument(
        "--max-depth",
        type=int,
        default=4,
        help="max depth to scan in --auto mode (default 4; higher is slow -- "
        "see module docstring)",
    )
    args = parser.parse_args()

    if args.auto:
        for i, (fen, expected) in enumerate(PERFT_TESTS, start=1):
            print(f"\n=== Position {i}: {fen} ===")
            depth = find_first_failing_depth(args.engine, fen, expected, args.max_depth)
            if depth is not None:
                print(f"\nBisecting position {i} starting at depth {depth}...")
                bisect(args.engine, fen, depth)
                return
        print("\nNo mismatches found in the scanned depths for any of the 6 positions.")
        print(
            f"(Only depths 1-{args.max_depth} were checked; raise --max-depth to check deeper, "
            "though it will be slower.)"
        )
        return

    if not args.fen or not args.depth:
        parser.error("either pass --auto, or both --fen and --depth")

    bisect(args.engine, args.fen, args.depth)


if __name__ == "__main__":
    main()
