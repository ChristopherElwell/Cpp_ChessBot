from pathlib import Path
import sys
import subprocess
import chess
import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk
import os
import time
import threading

colours = ["#DCE6C9", "#BCC6A9", "#FCF6E9"]


class window(tk.Tk):
    # Main window constructor
    def __init__(self, bot_dir):
        super().__init__()

        self.bot = start_bot(bot_dir)
        self.bot_thinking = False

        size = 500
        self.sqr_size = size / 8
        self.selected_piece = (-1, -1)
        self.geometry(f"{size+100}x{size}")
        self.title("Chess")
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

        # Status label for bot thinking indicator
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
                self.run_bot_async(self.bot, self.board.fen()),

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
        board = chess.Board()
        return board

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

    # Calls run_bot in a seperate thread so rendering is uninterrupted
    def run_bot_async(self, bot, fen):
        if self.bot_thinking:
            print("Bot is already thinking!")
            return

        thread = threading.Thread(target=self.run_bot, args=(bot, fen), daemon=True)
        thread.start()

    # Runs the bot by passing a position and a search time
    def run_bot(self, bot, fen):
        self.bot_thinking = True
        self.update_status("Bot thinking...", "yellow")

        print(f"Getting best move for pos:\n\t{fen}")
        bot_return = call_bot(bot, ["go", fen, "time", str(self.time_select.get())])

        self.after(0, self.handle_bot_response, bot_return)

    # Handles the bot response, expects: "bestmove [move uci]"
    def handle_bot_response(self, bot_return):
        self.bot_thinking = False
        self.update_status("Ready", "lightgreen")

        if bot_return is None:
            print("Error: No response from bot")
            return

        print(f"Bot Response:\n\t{bot_return}")
        bot_words = bot_return.split(" ")

        if bot_words[0] != "bestmove":
            print("Error: Invalid bot return")
            return

        try:
            move = chess.Move.from_uci(bot_words[1])
        except Exception as e:
            print(f"Error: Failed to convert to chess move: {bot_words[1]}")
            return

        if not move in self.board.legal_moves:
            print("Error: Bot move is not legal:", bot_return)
            return

        self.board.push(move)
        self.draw_board(self.get_piece_map(self.board))

    def update_status(self, text, color):
        self.status_label.config(text=text, bg=color)


# Calls the bot with a message, returns the response
def call_bot(process, messages):
    try:
        if process.poll() is not None:
            print(f"Bot is dead! Exit code: {process.returncode}")
            stderr_output = process.stderr.read()
            print(f"Stderr: {stderr_output}")
            return None

        process.stdin.write(" ".join(f"[{message}]" for message in messages) + "\n")
        process.stdin.flush()
        response = process.stdout.readline().strip()

        if response:
            return response
        else:
            print("Got empty response from bot")
            return None

    except OSError as e:
        print(f"OSError: {e}")
        if process.poll() is not None:
            stderr_output = process.stderr.read()
            print(f"Bot crashed. Stderr: {stderr_output}")
        return None


# Starts the bot application, and checks if its ready
def start_bot(bot_dir):
    process = subprocess.Popen(
        bot_dir,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
        cwd=Path(bot_dir).parent,
    )
    time.sleep(1)

    def stderr_reader():
        for line in iter(process.stderr.readline, ""):
            print(f"[BOT STDERR] {line.rstrip()}")

    threading.Thread(target=stderr_reader, daemon=True).start()

    ready_response = call_bot(process, ["ready"])
    print(f"Ready? Bot says: \n\t{ready_response}")

    return process


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python ElwellBotGui.py <bot_dir>")
        sys.exit(1)

    bot_dir = sys.argv[1]

    w = window(bot_dir)
    w.mainloop()
