import chess
import chess.polyglot
import sys

def bin_to_epd(book_path, epd_path):
    with chess.polyglot.open_reader(book_path) as reader, open(epd_path, "w") as epd_file:
        seen = set()
        for entry in reader:
            board = chess.Board()
            try:
                board.push(entry.move)
            except Exception:
                continue
            fen = board.fen()
            if fen not in seen:
                seen.add(fen)
                epd_file.write(fen + "\n")
        
    print(f"Exported {len(seen)} positions to {epd_path}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python bin.py to_epd.epd")
        sys.exit(1)

    bin_to_epd(sys.argv[1], sys.argv[2])
