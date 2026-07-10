use std::env;
use std::fs::File;
use std::io::{self, BufRead, BufReader, BufWriter, Write};
use std::str::FromStr;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ChessBoard {
    pub occ: u64,
    pub pcs: [u8; 16],
    pub score: i16,
    pub result: u8,
    pub ksq: u8,
    pub opp_ksq: u8,
    pub extra: [u8; 3],
}

const _RIGHT_SIZE: () = assert!(std::mem::size_of::<ChessBoard>() == 32);

impl ChessBoard {
    pub fn occ(&self) -> u64 {
        self.occ
    }

    pub fn our_ksq(&self) -> u8 {
        self.ksq
    }

    pub fn opp_ksq(&self) -> u8 {
        self.opp_ksq
    }

    pub fn extra(&self) -> [u8; 3] {
        self.extra
    }

    /// - Bitboards are in order White, Black, Pawn, Knight, Bishop, Rook, Queen, King.
    /// - Side-to-move is 0 for White, 1 for Black.
    /// - Score is White relative, in Centipawns.
    /// - Result is 0.0 for Black Win, 0.5 for Draw, 1.0 for White Win
    pub fn from_raw(
        mut bbs: [u64; 8],
        stm: usize,
        mut score: i16,
        mut result: f32,
    ) -> Result<Self, String> {
        if stm == 1 {
            for bb in bbs.iter_mut() {
                *bb = bb.swap_bytes();
            }

            bbs.swap(0, 1);

            score = -score;
            result = 1.0 - result;
        }

        let occ = bbs[0] | bbs[1];
        let mut pcs = [0; 16];

        let mut idx = 0;
        let mut occ2 = occ;
        while occ2 > 0 {
            let sq = occ2.trailing_zeros();
            let bit = 1 << sq;
            occ2 &= occ2 - 1;

            let colour = u8::from((bit & bbs[1]) > 0) << 3;
            let piece = bbs
                .iter()
                .skip(2)
                .position(|bb| bit & bb > 0)
                .ok_or("No Piece Found!".to_string())?;

            let pc = colour | piece as u8;

            pcs[idx / 2] |= pc << (4 * (idx & 1));

            idx += 1;
        }

        let result = (2.0 * result) as u8;
        let ksq = (bbs[0] & bbs[7]).trailing_zeros() as u8;
        let opp_ksq = (bbs[1] & bbs[7]).trailing_zeros() as u8 ^ 56;

        Ok(Self {
            occ,
            pcs,
            score,
            result,
            ksq,
            opp_ksq,
            extra: [0; 3],
        })
    }
}

impl std::str::FromStr for ChessBoard {
    type Err = String;

    fn from_str(s: &str) -> Result<Self, String> {
        let split: Vec<_> = s.split('|').collect();

        let fen = split[0];
        let score = split.get(1).ok_or("Malformed!")?.trim();
        let wdl = split.get(2).ok_or("Malformed!")?.trim();

        let parts: Vec<&str> = fen.split_whitespace().collect();
        let board_str = *parts.first().ok_or("Malformed FEN!")?;
        let stm_str = *parts.get(1).ok_or("Malformed FEN!")?;

        let stm = u8::from(stm_str == "b");

        let mut board = Self::default();

        let mut idx = 0;

        let mut parse_row = |i: usize, row: &str| {
            let mut col = 0;
            for ch in row.chars() {
                if ('1'..='8').contains(&ch) {
                    col += ch.to_digit(10).expect("hard coded") as usize;
                } else if let Some(mut piece) = "PNBRQKpnbrqk".chars().position(|el| el == ch) {
                    let mut square = 8 * i + col;

                    piece = (piece / 6) << 3 | (piece % 6);

                    // black to move
                    if stm == 1 {
                        piece ^= 8;
                        square ^= 56;
                    }

                    if piece == 5 {
                        board.ksq = square as u8;
                    }

                    if piece == 13 {
                        board.opp_ksq = square as u8 ^ 56;
                    }

                    board.occ |= 1 << square;

                    if idx >= 32 {
                        return Err(s);
                    }

                    board.pcs[idx / 2] |= (piece as u8) << (4 * (idx & 1));
                    idx += 1;
                    col += 1;
                }
            }
            Ok(())
        };

        if stm == 1 {
            for (i, row) in board_str.split('/').enumerate() {
                parse_row(7 - i, row)?;
            }
        } else {
            for (i, row) in board_str.split('/').rev().enumerate() {
                parse_row(i, row)?;
            }
        }

        board.score = if let Ok(x) = score.parse::<i16>() {
            x
        } else {
            println!("{s}");
            return Err(String::from("Bad score!"));
        };

        board.result = match wdl {
            "1.0" | "[1.0]" | "1" => 2,
            "0.5" | "[0.5]" | "1/2" => 1,
            "0.0" | "[0.0]" | "0" => 0,
            _ => {
                println!("{s}");
                return Err(String::from("Bad game result!"));
            }
        };

        if stm == 1 {
            board.score = -board.score;
            board.result = 2 - board.result;
        }

        Ok(board)
    }
}

fn main() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        eprintln!("Usage: {} <path_to_fen_file>", args[0]);
        std::process::exit(1);
    }

    let file_path = &args[1];

    let file = File::open(file_path)?;
    let reader = BufReader::new(file);

    const OUTPUT_PATH: &str = "scripts/assets/bf";

    let output = File::create(OUTPUT_PATH)?;
    let mut writer = BufWriter::new(output);

    let mut line_counter = 0;

    const FIXED_SCORE: i32 = 120;
    const FIXED_RESULT: &str = "0";

    for line_result in reader.lines() {
        line_counter += 1;
        let line = line_result?;
        
        if line.is_empty() {
            continue;
        }

        let line_game_data = format!("{}|{}|{}", line, FIXED_SCORE, FIXED_RESULT);

        match ChessBoard::from_str(&line_game_data) {
            Ok(board) => {
                let raw_bytes = unsafe {
                    std::slice::from_raw_parts(
                        &board as *const ChessBoard as *const u8,
                        std::mem::size_of::<ChessBoard>(), // = 32
                    )
                };

                writer.write_all(raw_bytes)?;
            }
            Err(e) => {
                eprintln!("Error parsing line {line_counter}: {e} -> \"{line}\"");
                continue;
            }
        }
    }

    writer.flush()?;

    Ok(())
}