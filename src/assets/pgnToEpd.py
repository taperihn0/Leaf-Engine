import chess.pgn
import sys

def pgn_to_epd(input_pgn, output_epd, skip_duplicates = True):
    seen = set()

    with open(input_pgn, "r", encoding="utf-8") as pgn_file, \
         open(output_epd, "w", encoding="utf-8") as epd_file:

        while True:
            game = chess.pgn.read_game(pgn_file)
            if game is None:
                break

            board = game.board()
            for move in game.mainline_moves():
                board.push(move)
                fen = board.fen()

                if skip_duplicates:
                    if fen in seen:
                        continue
                    seen.add(fen)

                epd_file.write(fen + "\n")

    print(f"Done. EPD saved to {output_epd}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python pgn_to_epd.py input.pgn output.epd")
        sys.exit(1)

    pgn_to_epd(sys.argv[1], sys.argv[2])
