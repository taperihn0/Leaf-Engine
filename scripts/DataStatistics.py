import os
import sys
import signal
import numpy as np
from pathlib import Path
from dataclasses import dataclass

# Zaktualizowany rozmiar rekordu: 8 (occ) + 16 (pieces) + 8 (PackedPosInfo) = 32
RECORD_SIZE = 32

@dataclass
class Stats:
    total_positions: int = 0
    white_wins: int = 0
    black_wins: int = 0
    draws: int = 0
    
    score_sum_white_win: float = 0
    score_sum_black_win: float = 0
    score_sum_draw: float = 0

    relative_pos_count: int = 0

global_stats = Stats()
is_running = True

def print_statistics():
    """Generuje raport statystyczny."""
    print("\n" + "="*50)
    print("      TDF DATASET STATISTICS REPORT      ")
    print("="*50)
    
    if global_stats.total_positions == 0:
        print("No data processed.")
        return

    t = global_stats.total_positions
    ww = global_stats.white_wins
    bw = global_stats.black_wins
    dw = global_stats.draws

    print(f"Total Positions Parsed : {t:,}")
    print("-" * 50)
    
    print(f"White Wins             : {ww:,} ({(ww/t)*100:.2f}%)")
    print(f"Black Wins             : {bw:,} ({(bw/t)*100:.2f}%)")
    print(f"Draws                  : {dw:,} ({(dw/t)*100:.2f}%)")
    
    print("-" * 50)
    if bw > 0:
        print(f"Win Ratio (White/Black): {ww/bw:.3f}")
    else:
        print("Win Ratio (White/Black): N/A")
        
    print("-" * 50)
    
    avg_ww_score = global_stats.score_sum_white_win / ww if ww > 0 else 0
    avg_bw_score = global_stats.score_sum_black_win / bw if bw > 0 else 0
    avg_dw_score = global_stats.score_sum_draw / dw if dw > 0 else 0
    
    print(f"Avg Score (White Win)  : {avg_ww_score:+.2f} cp")
    print(f"Avg Score (Black Win)  : {avg_bw_score:+.2f} cp")
    print(f"Avg Score (Draw)       : {avg_dw_score:+.2f} cp")
    print("-" * 50)
    print(f"Relative flags set     : {global_stats.relative_pos_count:,}")
    print("="*50 + "\n")

def signal_handler(sig, frame):
    global is_running
    print("\n[!] Zatrzymywanie... Generuję statystyki końcowe.")
    is_running = False

def process_file(filepath: Path):
    global global_stats, is_running
    
    file_size = os.path.getsize(filepath)
    if file_size % RECORD_SIZE != 0:
        print(f"\n[Warning] Plik {filepath.name} ({file_size} bytes) ma resztę z dzielenia przez {RECORD_SIZE}. Ostatnie wpisy mogą być ucięte.")
    
    num_records = file_size // RECORD_SIZE
    if num_records == 0:
        return

    # Poprawiony Memory Layout (dokładnie 32 bajty)
    dt = np.dtype([
        ('occupancy', '<u8'),   # 8 bajtów
        ('pieces', '16V'),      # 16 bajtów
        ('score', '<i2'),       # 2 bajty
        ('result', '<u1'),      # 1 bajt
        ('ksq', '<u1'),         # 1 bajt
        ('opp_ksq', '<u1'),     # 1 bajt
        ('relative', '?'),      # 1 bajt (bool)
        ('pad', '2V')           # 2 bajty wypełnienia (__align)
    ])                          # Razem: 32 bajty
    
    try:
        data = np.memmap(filepath, dtype=dt, mode='r', shape=(num_records,))
        
        global_stats.total_positions += num_records
        
        results = data['result']
        scores = data['score']
        
        black_wins_mask = (results == 0)
        draws_mask = (results == 1)
        white_wins_mask = (results == 2)
        
        bw_count = np.count_nonzero(black_wins_mask)
        dw_count = np.count_nonzero(draws_mask)
        ww_count = np.count_nonzero(white_wins_mask)
        
        global_stats.black_wins += bw_count
        global_stats.draws += dw_count
        global_stats.white_wins += ww_count
        
        if bw_count > 0: global_stats.score_sum_black_win += np.sum(scores[black_wins_mask])
        if dw_count > 0: global_stats.score_sum_draw += np.sum(scores[draws_mask])
        if ww_count > 0: global_stats.score_sum_white_win += np.sum(scores[white_wins_mask])
            
        global_stats.relative_pos_count += np.count_nonzero(data['relative'])

    except Exception as e:
        print(f"\n[Error] Nie udało się przetworzyć pliku {filepath.name}: {e}")

def main():
    if len(sys.argv) < 2:
        print("Użycie: python tdf_analyzer.py <ścieżka_do_folderu>")
        sys.exit(1)

    target_dir = Path(sys.argv[1])
    if not target_dir.is_dir():
        print(f"Błąd: Folder '{target_dir}' nie istnieje.")
        sys.exit(1)

    signal.signal(signal.SIGINT, signal_handler)

    tdf_files = list(target_dir.rglob("*.tdf"))
    print(f"Znaleziono {len(tdf_files)} plików .tdf.")
    print("Wciśnij Ctrl+C aby w dowolnym momencie przerwać i wyświetlić statystyki.\n")

    for idx, filepath in enumerate(tdf_files):
        if not is_running:
            break
            
        sys.stdout.write(f"\rPrzetwarzanie pliku {idx + 1}/{len(tdf_files)}: {filepath.name} ...")
        sys.stdout.flush()
        
        process_file(filepath)

    print("\n")
    print_statistics()

if __name__ == "__main__":
    main()
