<p align="center">
  <img src="https://github.com/user-attachments/assets/3eed7bfb-19d8-4373-8f82-7394b8eeae79" alt="Leaf logo" width="300">
</p>

<h1 align="center">Leaf</h1>

<p align="center">A NNUE chess engine written from scratch in C++17.</p>

---

Leaf is built around a traditional alpha-beta search with handwritten heuristics rather than borrowed frameworks, and the codebase is written and maintained almost entirely by hand — very little of it is AI-generated.

## Architecture & features

- **Search** — Principal Variation Search with handcrafted move ordering and pruning heuristics.
- **Evaluation** — Own NNUE net, with hand-written SIMD (AVX2/NEON) for the Lizard SCReLU activation.
- **Move generation** — Bitboard-based, supporting both pseudo-legal and fully legal generation.
- **Network weights** — Embedded directly into the executable; no external `.bin` file needed at runtime.
- **Endgame tablebases** — Syzygy support via [Fathom](https://github.com/jdart1/Fathom/tree/master).
- **Tuning** — Multithreaded SPSA framework built in for search parameter optimization.
- **Data generation** — Integrated self-play pipeline for producing NNUE training data.

## Search & heuristics

The search is PVS with iterative deepening and aspiration windows on top. Pruning and reduction techniques currently in use:

- Null move pruning with dynamic verification
- Reverse futility pruning, razoring, futility pruning
- SEE-based pruning
- Late move reductions, tuned separately for quiets, captures, and checks
- Singular extensions with dedicated beta margins
- Dynamic contempt, internal iterative deepening, quiescence search

Most of the depth and pruning parameters are tuned via the SPSA framework rather than by hand.

## Installation

Prebuilt binaries for Windows and Linux are available under [Releases](../../releases) — building from source isn't required to use the engine.

## Neural network

Leaf uses a custom binary weight format built on the `.bin` layout from [Bullet](https://github.com/jw1912/bullet/tree/main).

- **Architecture:** (768→128)x2→1
- **Data:** trained on self-play data in [bullet-format](https://github.com/jw1912/bullet/blob/main/docs/3-data.md), over 200 million positions so far, generated at a 7k-node limit per game from randomized openings. Data packing is on the list of things to improve.

## Project layout

```text
Leaf/
├── assets/
│   ├── books/             # Internal opening books
│   ├── nets/              # NNUE binary weight files (.bin)
│   └── tb/                # Syzygy tablebases
├── misc/                  # Miscellaneous docs
├── scripts/               # Automation scripts
├── src/
│   ├── backend/           # Core engine
│   │   └── win-embed/     # Resource scripts for Windows embedding
│   ├── frontend/          # UCI protocol handling
│   ├── utils/             # Helper tools and data generators
│   └── vendor/            # Third-party libraries
└── CMakeLists.txt
```

## Credits

Leaf wouldn't exist without the chess programming community.

- [Bullet contributors](https://github.com/jw1912/bullet/graphs/contributors?from=4%2F25%2F2026) for the training platform
- [Maksim Korzh's BBC tutorials](https://www.youtube.com/playlist?list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs), which got me started on my previous engine
- The [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page)
- [Paweł Kozioł / Publius](https://github.com/nescitus/publius) for the starting-ground net
- Everyone on the Engine Programming Discord who's answered questions along the way

## License

GNU General Public License (GPL).