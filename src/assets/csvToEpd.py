import csv

def extract_fens(input_csv, output_epd):
    with open(input_csv, newline='', encoding='utf-8') as csvfile:
        reader = csv.reader(csvfile, delimiter=',')
        with open(output_epd, 'w', encoding='utf-8') as epdfile:
            for row in reader:
                if not row:
                    continue
                fen = row[0].strip()
                if fen:
                    epdfile.write(fen + "\n")

if __name__ == "__main__":
    extract_fens("positions.csv", "positions.epd")
