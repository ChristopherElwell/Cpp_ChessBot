from pathlib import Path
import re
import time
import threading
import argparse
from datetime import datetime

import chess
import chess.engine
import chess.pgn
import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk

colours = ["#DCE6C9", "#BCC6A9", "#FCF6E9"]


# ---------------------------------------------------------------------------
# Engine helpers
#
# Every engine now speaks (a subset of) standard UCI: "uci"/"uciok",
# "isready"/"readyok", "position ...", "go movetime <ms>" -> "bestmove ...",
# "stop", "quit". Because of that, we can drive your bot *and* Stockfish
# through python-chess's own UCI client (chess.engine) instead of hand
# rolling a pipe protocol. That's what makes bot-vs-bot and bot-vs-Stockfish
# the same code path below.
# ---------------------------------------------------------------------------


def start_engine(path):
    """Starts a UCI engine and returns (engine, display_name).

    display_name comes from the engine's own "id name" response so PGNs
    and filenames are labelled with whatever the engine calls itself.
    """
    engine = chess.engine.SimpleEngine.popen_uci(path, cwd=Path(path).parent)
    name = engine.id.get("name") or Path(path).stem
    return engine, name


def configure_strength(engine, name, skill_level=None, elo=None):
    """Best-effort strength config. Mainly meant for Stockfish's
    "Skill Level" / "UCI_Elo" options, but harmless to try on any engine -
    if it doesn't support the option we just warn and move on."""
    try:
        if elo is not None:
            engine.configure({"UCI_LimitStrength": True, "UCI_Elo": elo})
            print(f"{name}: strength limited to ~{elo} Elo")
        elif skill_level is not None:
            engine.configure({"Skill Level": skill_level})
            print(f"{name}: Skill Level set to {skill_level}")
    except chess.engine.EngineError as e:
        print(f"Warning: {name} doesn't support that strength option: {e}")


def sanitize(name):
    return re.sub(r"[^A-Za-z0-9_-]+", "", name.replace(" ", "")) or "engine"


def auto_pgn_filename(name1, name2, num_games):
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    return f"game_results/{stamp}_{sanitize(name1)}_vs_{sanitize(name2)}_{num_games}games.pgn"


# ---------------------------------------------------------------------------
# GUI: human vs one UCI engine
# ---------------------------------------------------------------------------


class ChessWindow(tk.Tk):
    def __init__(self, bot_dir):
        super().__init__()

        self.engine, self.engine_name = start_engine(bot_dir)
        self.bot_thinking = False

        size = 500
        self.sqr_size = size / 8
        self.selected_piece = None
        self.geometry(f"{size + 100}x{size}")
        self.title(f"Chess vs {self.engine_name}")
        self.canvas = tk.Canvas(border=None)
        self.canvas.place(x=0, y=0, width=size, height=size, anchor="nw")

        self.FENbutton = tk.Button(
            self, text="Print FEN", command=lambda: print(self.board.fen())
        )
        self.FENbutton.place(x=size, y=0, width=100, height=60, anchor="nw")

        self.input_fen_btn = tk.Button(self, text="Input Fen", command=self.input_fen)
        self.input_fen_btn.place(x=size, y=size / 4, width=100, height=60, anchor="nw")

        times = [1, 2, 3, 5, 10, 15, 20, 30, 60]
        self.time_select = ttk.Combobox(
            self, values=[str(v) for v in times], state="readonly"
        )
        self.time_select.current(2)
        self.time_select.place(
            x=size, y=size * 3 / 4, width=100, height=60, anchor="nw"
        )

        self.status_label = tk.Label(self, text="Ready", bg="lightgreen")
        self.status_label.place(x=size, y=size - 40, width=100, height=40, anchor="nw")

        piece_image_path_map = {
            "p": "Pieces\\black-pawn.png",
            "n": "Pieces\\black-knight.png",
            "b": "Pieces\\black-bishop.png",
            "r": "Pieces\\black-rook.png",
            "k": "Pieces\\black-king.png",
            "q": "Pieces\\black-queen.png",
            "P": "Pieces\\white-pawn.png",
            "N": "Pieces\\white-knight.png",
            "B": "Pieces\\white-bishop.png",
            "R": "Pieces\\white-rook.png",
            "K": "Pieces\\white-king.png",
            "Q": "Pieces\\white-queen.png",
        }
        self.piece_img_map = {
            piece_char: ImageTk.PhotoImage(
                Image.open(piece_path)
                .convert("RGBA")
                .resize((int(self.sqr_size), int(self.sqr_size)), Image.LANCZOS)
            )
            for piece_char, piece_path in piece_image_path_map.items()
        }

        self.board = chess.Board()
        self.draw_board(self.get_piece_map(self.board))

        self.bind("<Button-1>", func=lambda x: self.on_click(x, self.board))
        self.protocol("WM_DELETE_WINDOW", self.on_close)

    def on_close(self):
        try:
            self.engine.quit()
        except Exception:
            pass
        self.destroy()

    # Copies a FEN from the users clipboard and sets the board to it
    def input_fen(self):
        self.board = chess.Board(fen=self.clipboard_get())
        self.draw_board(self.get_piece_map(self.board))

    # Handles all events caused by user clicking
    def on_click(self, event, board):
        if self.bot_thinking:
            return  # Ignore clicks while bot is thinking

        sq = (int(event.x // self.sqr_size), int(7 - event.y // self.sqr_size))
        if (
            board.piece_at(sq[0] + sq[1] * 8) is not None
            and board.piece_at(sq[0] + sq[1] * 8).color == board.turn
        ):
            self.selected_piece = sq
        elif self.selected_piece is not None:
            move = chess.Move(chess.square(*self.selected_piece), chess.square(*sq))

            piece = board.piece_at(chess.square(*self.selected_piece))
            if (
                piece
                and piece.piece_type == chess.PAWN
                and (
                    (board.turn == chess.WHITE and sq[1] == 7)
                    or (board.turn == chess.BLACK and sq[1] == 0)
                )
            ):
                move = chess.Move(
                    chess.square(*self.selected_piece),
                    chess.square(*sq),
                    promotion=chess.QUEEN,
                )

            if board.is_legal(move):
                board.push(move)
                self.selected_piece = None
                self.run_bot_async()

        if board.is_checkmate():
            print("CHECKMATE")
        elif board.is_stalemate():
            print("STALEMATE")

        self.draw_board(self.get_piece_map(board))

    # Draws the board's grid to screen
    def make_grid(self):
        for i in range(8):
            for j in range(8):
                self.canvas.create_rectangle(
                    self.sqr_size * i,
                    self.sqr_size * j,
                    self.sqr_size * (i + 1),
                    self.sqr_size * (j + 1),
                    fill=colours[(i + j) % 2],
                    outline=colours[(i + j) % 2],
                )

    # Returns a dictionary of position and pieces representing the board state
    def get_piece_map(self, board):
        return {key: piece.symbol() for key, piece in board.piece_map().items()}

    # Draws the board to the screen
    def draw_board(self, pieces):
        self.canvas.delete("all")
        self.make_grid()
        if self.selected_piece is not None:
            self.canvas.create_rectangle(
                self.sqr_size * (self.selected_piece[0]),
                self.sqr_size * (8 - self.selected_piece[1]),
                self.sqr_size * ((self.selected_piece[0] + 1)),
                self.sqr_size * (8 - (self.selected_piece[1] + 1)),
                fill=colours[2],
                outline=colours[2],
            )
        for sqr, pc in pieces.items():
            self.canvas.create_image(
                (sqr % 8 + 0.5) * self.sqr_size,
                (7 - sqr // 8 + 0.5) * self.sqr_size,
                image=self.piece_img_map[pc],
                anchor="center",
            )

    # Calls run_bot in a separate thread so rendering is uninterrupted
    def run_bot_async(self):
        if self.bot_thinking:
            print("Bot is already thinking!")
            return

        thread = threading.Thread(target=self.run_bot, daemon=True)
        thread.start()

    # Asks the engine (via UCI "go movetime") for its move on the current position
    def run_bot(self):
        self.bot_thinking = True
        self.update_status("Bot thinking...", "yellow")

        movetime = float(self.time_select.get())
        fen = self.board.fen()
        print(f"Getting best move for pos:\n\t{fen}")

        move = None
        try:
            result = self.engine.play(self.board, chess.engine.Limit(time=movetime))
            move = result.move
        except chess.engine.EngineError as e:
            print(f"Error: engine error: {e}")

        self.after(0, self.handle_bot_response, move)

    # Handles the engine's move once it's back on the main thread
    def handle_bot_response(self, move):
        self.bot_thinking = False
        self.update_status("Ready", "lightgreen")

        if move is None:
            print("Error: no move from bot")
            return

        if move not in self.board.legal_moves:
            print("Error: bot move is not legal:", move)
            return

        print(f"Bot plays: {move}")
        self.board.push(move)
        self.draw_board(self.get_piece_map(self.board))

    def update_status(self, text, color):
        self.status_label.config(text=text, bg=color)


# ---------------------------------------------------------------------------
# Headless fight club: engine1 vs engine2 (your bot vs Stockfish, or
# your bot vs another build of itself - it's all just UCI now)
# ---------------------------------------------------------------------------


def run_match(
    engine1_path,
    engine2_path,
    num_games=10,
    time1=3.0,
    time2=3.0,
    skill_level=None,
    elo=None,
    engine1_starts=True,
    pgn_file=None,
):
    engine1, name1 = start_engine(engine1_path)
    engine2, name2 = start_engine(engine2_path)

    if name1 == name2:
        # Keep results/headers unambiguous if you're pitting the bot
        # against an identically-named build of itself
        name1, name2 = f"{name1} (1)", f"{name2} (2)"

    # Best-effort strength config on engine2 - a no-op if it's another
    # build of your own bot that doesn't expose these UCI options
    configure_strength(engine2, name2, skill_level, elo)

    if pgn_file is None:
        pgn_file = auto_pgn_filename(name1, name2, num_games)
    pgn_out = open(pgn_file, "a", encoding="utf-8", newline="\n")

    results = {name1: 0, name2: 0, "draws": 0}
    engine1_is_white = engine1_starts

    try:
        for game_num in range(1, num_games + 1):
            board = chess.Board()
            print(
                f"\n=== Game {game_num}/{num_games}: "
                f"{name1} is {'White' if engine1_is_white else 'Black'}, "
                f"{name2} is {'White' if not engine1_is_white else 'Black'} ==="
            )

            aborted = False
            while not board.is_game_over(claim_draw=True):
                engine1_turn = (board.turn == chess.WHITE) == engine1_is_white
                engine, movetime, mover_name = (
                    (engine1, time1, name1) if engine1_turn else (engine2, time2, name2)
                )

                try:
                    result = engine.play(board, chess.engine.Limit(time=movetime))
                except chess.engine.EngineError as e:
                    print(f"{mover_name} failed to move ({e}), aborting game")
                    aborted = True
                    break

                if result.move is None or result.move not in board.legal_moves:
                    print(f"{mover_name} returned an illegal/empty move, aborting game")
                    aborted = True
                    break

                board.push(result.move)
                print(f"{mover_name} plays: {result.move}")

            if aborted:
                print(f"Final FEN: {board.fen()}")
            else:
                outcome = board.outcome(claim_draw=True)
                if outcome.winner is None:
                    print(f"Result: Draw ({outcome.termination.name})")
                    results["draws"] += 1
                else:
                    winner_name = name1 if outcome.winner == engine1_is_white else name2
                    print(f"Result: {winner_name} wins ({outcome.termination.name})")
                    results[winner_name] += 1

                pgn_game = chess.pgn.Game.from_board(board)
                pgn_game.headers["Event"] = f"{name1} vs {name2}"
                pgn_game.headers["Round"] = str(game_num)
                pgn_game.headers["White"] = name1 if engine1_is_white else name2
                pgn_game.headers["Black"] = name2 if engine1_is_white else name1
                pgn_game.headers["Result"] = outcome.result()

                exporter = chess.pgn.FileExporter(pgn_out)
                pgn_game.accept(exporter)
                pgn_out.flush()
                print(f"Game {game_num} saved to {pgn_file}")

            engine1_is_white = not engine1_is_white  # alternate colours each game

    finally:
        for e in (engine1, engine2):
            try:
                e.quit()
            except Exception:
                pass
        pgn_out.close()

    print("\n=== Match Results ===")
    print(f"{name1} wins: {results[name1]}")
    print(f"{name2} wins: {results[name2]}")
    print(f"Draws:       {results['draws']}")
    print(f"Games saved to: {pgn_file}")

    return results


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Chess GUI (human vs your bot), or a headless UCI fight "
        "club: your bot vs Stockfish, or your bot vs another build of itself."
    )
    parser.add_argument(
        "engine1",
        help="Path to a UCI engine executable (your bot). Also the opponent "
        "used in GUI mode.",
    )

    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--stockfish",
        "-s",
        default=None,
        help="Path to stockfish.exe -> run engine1 vs Stockfish",
    )
    group.add_argument(
        "--bot2",
        "-b2",
        default=None,
        help="Path to a second UCI bot -> run engine1 vs bot2",
    )

    parser.add_argument(
        "--games", "-g", type=int, default=10, help="Number of games (default: 10)"
    )
    parser.add_argument(
        "--time1",
        type=float,
        default=3.0,
        help="Seconds per move for engine1 (default: 3.0)",
    )
    parser.add_argument(
        "--time2",
        type=float,
        default=None,
        help="Seconds per move for engine2/Stockfish (default: same as "
        "--time1, or 0.1 if --stockfish is used)",
    )
    parser.add_argument(
        "--skill-level",
        type=int,
        default=None,
        help="Stockfish 'Skill Level' UCI option, 0 (weakest) to 20 (strongest)",
    )
    parser.add_argument(
        "--elo",
        type=int,
        default=None,
        help="Limit Stockfish to roughly this Elo instead of --skill-level "
        "(overrides --skill-level if both given)",
    )
    parser.add_argument(
        "--bot2-starts",
        action="store_true",
        help="Have engine2/Stockfish play White in game 1 instead of engine1",
    )
    parser.add_argument(
        "--pgn-file",
        default=None,
        help="Override the auto-generated PGN filename (default: "
        "<timestamp>_<name1>_vs_<name2>_<N>games.pgn)",
    )

    args = parser.parse_args()

    if args.stockfish or args.bot2:
        opponent_path = args.stockfish or args.bot2
        time2 = args.time2
        if time2 is None:
            time2 = 0.1 if args.stockfish else args.time1

        run_match(
            args.engine1,
            opponent_path,
            num_games=args.games,
            time1=args.time1,
            time2=time2,
            skill_level=args.skill_level,
            elo=args.elo,
            engine1_starts=not args.bot2_starts,
            pgn_file=args.pgn_file,
        )
    else:
        w = ChessWindow(args.engine1)
        w.mainloop()
