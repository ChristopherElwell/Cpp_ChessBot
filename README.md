# ElwellBot

**ElwellBot** is a C++ chess engine that can be run either as a command-line engine or through a Python GUI (`ElwellBotGui.py`).  It supports standard chess move generation, search, and evaluation.

## Features

- Bitboard-based move generation  
- Evaluation function using piece-square tables  
- Alpha-beta pruning with move ordering  
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
pip install pillow chess tk
```
## Build
- Configure for a debug build:

```bash
cmake -B [build folder] -DCMAKE_BUILD_TYPE=Debug
```

- ... or for a release build:

```bash
cmake -B [build folder] -DCMAKE_BUILD_TYPE=Release
```
- Compile the bot:

```bash
cmake --build [build folder]
```
- Run the bot, either by running the executable direclty and controlling via command line, or running `pythong ElwellBotGui.py [path to bot]`. 

   The bot will be found in `[build folder] / [Debug/Release]`. 