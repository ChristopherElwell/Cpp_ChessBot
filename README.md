# ElwellBot

**ElwellBot** is a C++ chess engine that can be run either as a command-line engine or through a Python GUI (`ElwellBotGui.py`). It supports standard chess move generation, search, and evaluation.

## Features

- Bitboard-based move generation
- Evaluation function using piece-square tables
- Alpha-beta pruning with move ordering
- UCI protocol support
- Built-in perft, puzzle, and debugging tools
- Python GUI frontend for interactive play or testing

---

## Requirements

### C++ Build
- CMake ≥ 3.14
- C++23-compatible compiler

### Python GUI
- Python 3.10+
- Dependencies:
```bash
pip install pillow chess
```
`tkinter` ships with most Python installs. On some Linux distros you need to install it separately (e.g. `sudo apt install python3-tk`).

---

## Build

Configure for a debug build:

```bash
cmake -B [build folder] -DCMAKE_BUILD_TYPE=Debug
```

...or for a release build:

```bash
cmake -B [build folder] -DCMAKE_BUILD_TYPE=Release
```

Compile the bot:

```bash
cmake --build [build folder]
```

The executable will be found in `[build folder]/[Debug|Release]`, depending on your generator.

---

## Running

Run the executable directly and control it from the command line, or launch the GUI:

```bash
python ElwellBotGui.py [path to bot]
```

## Command-Line Usage

```
chess [options]
```

| Command | Description |
|---|---|
| `chess` | Run UCI mode |
| `chess --uci` | Run UCI mode |
| `chess --puzzles N` | Run N puzzles |
| `chess --perft N` | Run perft to depth N |
| `chess --conversion` | Run the move conversion test |
| `chess --ttable` | Run the move transposition table test |
| `chess --help` | Show help |

Test flags can be combined to run multiple tests in one invocation:

```bash
chess --conversion --perft 5
```

### Diagnostics

These are one-off debugging tools. They must be used on their own and can't be combined with other flags.

| Command | Description |
|---|---|
| `chess --divide FEN DEPTH` | Show per-move perft counts ("divide") at the given FEN and depth, for debugging move generation. See `perft_debug.py`. |
| `chess --checkmove FEN UCI` | Compare the incrementally updated state against a freshly parsed one after applying the UCI move to the FEN. Catches `apply_move` state-sync bugs. |
| `chess --verify FEN DEPTH` | Walk the perft tree from the FEN to the given depth, checking that every apply/undo round trip restores the board exactly. Reports the move path to the first bad undo, if any. |

Quote the FEN so your shell treats it as a single argument:

```bash
chess --divide "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" 4
```
