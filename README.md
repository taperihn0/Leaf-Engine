<p align="center">
  <img src="https://github.com/user-attachments/assets/67c17d0c-b990-48a0-a0a3-74f4bcbc80b0" alt="Leaf logo" width="242">
</p>

<h1 align="center">Leaf</h1>

<p align="center">Hobbyist NNUE Chess Engine written from scratch</p>

---

Leaf arised from my interest in chess programming. It is built upon traditional, fine tuned Alpha-Beta search and runs completelly on CPU. Leaf is empowered by [Efficiently Updatable Neural Network](https://en.wikipedia.org/wiki/Efficiently_updatable_neural_network) that is activelly trained on self-play data. Leaf infrastructure is written in C++.

## 🧠 Architecture

- **Search** — Principal Variation Search & Internal Iterative Deepening with handcrafted move ordering and pruning heuristics.
- **Neural Network** — Embedded directly into the executable; no external `.bin` file needed at runtime.
- **Endgame tablebases** — Syzygy support via [Fathom](https://github.com/jdart1/Fathom/tree/master).
- **Tuning** — Multithreaded SPSA framework built in Leaf's internal tools for search parameter optimization.

## 🔮 Neural network

Leaf uses a custom binary weight format built on the `.bin` layout from [Bullet](https://github.com/jw1912/bullet/tree/main).

- **Architecture:** (768→128)x2→1
- **Data:** trained on self-play data in [bullet-format](https://github.com/jw1912/bullet/blob/main/docs/3-data.md), over 200 million positions so far, generated at a 8k-node limit per game from randomized openings. Data packing is on the list of things to improve.

## 📁 Project layout

```text
Leaf/
├── assets/
│   ├── books/             # Internal opening books
│   ├── nets/              # Neural net binary weight files with Leaf-compatible headers (.bin)
│   └── tb/                # Tablebases
├── misc/                  # Miscellaneous docs and logs
├── scripts/               # Automation scripts
├── src/
│   ├── backend/           # Core source
│   ├── frontend/          # UCI protocol
│   ├── utils/             # Internal developing tools
│   └── vendor/            # Third-party libraries
└── CMakeLists.txt
```

## 📦 Installation

Prebuilt binaries for Windows and Linux are available under [Releases](../../releases). For now, only AVX2 binaries are available. Legacy hardware and ARM architecture support in the near future.

## 🤝 Credits

I wouldn't be spending hours developing Leaf without the passionate community of chess engine developers.

* **[Bullet contributors](https://github.com/jw1912/bullet/graphs/contributors?from=4%2F25%2F2026)** for providing excellent training platform.
* **[BBC Chess Engine Tutorials by Maksim Korzh](https://www.youtube.com/playlist?list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs)**
whose tutorials were an introduction to the chess programming world and which helped me developing
my previous engine from scratch.
* **[ChessProgramming Wiki Team](https://www.chessprogramming.org/Main_Page)** 
for the entire knowledge concentrated in one place.
* **[Publius Chess Engine, especially Paweł Kozioł](https://github.com/nescitus/publius)** for providing starting-ground net.
* **As well as all the helpful developers from Engine Programming Discord Group.**

### License

GNU General Public License (GPL).