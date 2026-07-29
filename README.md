# 🍃 Leaf Chess Engine 🍃

**Leaf** is a modern, NNUE chess engine written from scratch in **C++17**. The project was created with a strong focus on maximum performance, clean architecture and friendly development workflow. Leaf codebase is developed and maintained almost entirelly without AI generated code.

---

## 🏗️ Architecture & Features

- ♟️ **Traditional Alpha-Beta Search Framework** – Principal Variation Search platform with handcrafted heuristics.
- 🧠 **NNUE Evaluation** – Lightning-fast evaluation featuring **handwritten SIMD code (AVX2/NEON) for the Lizard SCReLU** activation function.
- ⚡ **Custom Move Generator** – A highly efficient, bitboard-based generator supporting both **pseudo-legal** and **fully legal** move generation.
- 📦 **Embedded Neural Network** – Network weights are seamlessly injected directly into the executable, requiring no external `.bin` files to run.
- 📚 **Syzygy Tablebase Support** – Endgame tablebase probing using **[Fathom](https://github.com/jdart1/Fathom/tree/master)** library.
- ⚙️ **Multithreaded SPSA Tuning** – A built-in parallelized framework for automated search parameter optimization.
- 🗄️ **Custom Data Collector** – A fully integrated pipeline generating self-play data for neural network training.

---

## 🧠 Search & Heuristics

Leaf's search algorithm is built on Principal Variation Search (PVS) combined with Iterative Deepening and Aspiration Windows. Key pruning and extension heuristics include:

- **Pruning:** Null Move Pruning (NMP) with dynamic verification, Reverse Futility Pruning (RFP), Razoring, Futility Pruning, and SEE Pruning.
- **Reductions & Extensions:** Late Move Reductions (LMR) adjusted for move types (quiets / captures / checks), and Singular Extensions with dedicated Beta margins.
- **Other Techniques:** Dynamic Contempt Factor, Internal Iterative Deepening (IID), Quiescence Search (QS).

*Most critical depth and pruning parameters are optimized using an integrated SPSA tuning framework.*

---

## 📥 Installation (Binaries)

You do not need to build the engine from source to use it. 
Ready-to-use, compiled binaries for both Windows and Linux are available in the **[Releases](../../releases)** tab of this repository. 

---

## 🔮 Neural Network (NNUE)

Leaf utilizes a custom binary format for its NNUE weights built upon `.bin` format utilized in **[Bullet](https://github.com/jw1912/bullet/tree/main)** trainer.
- **Architecture:** Simple **(768->128)x2->1** architecture.
- **Data Pipeline:** Full support for custom dataset generation exported in the [bullet-format](https://github.com/jw1912/bullet/blob/main/docs/3-data.md) (>200 million positions evaluated so far). There are some plans to use more optimized data packs.

---

## 📁 Project Overview

```text
Leaf/
├── assets/
│   ├── books/             # Internal opening books
│   ├── nets/              # NNUE binary weight files (.bin)
│   └── tb/                # Syzygy Tablebases
├── misc/                  # Miscellaneous docs
├── scripts/               # Automation scripts
├── src/
│   ├── backend/           # Core engine 
│   │   └── win-embed/     # Resource scripts for Windows embeddings
│   ├── frontend/          # User communication - UCI Protocol
│   ├── utils/             # Helper tools and data generators
│   └── vendor/            # Third-party libraries
└── CMakeLists.txt         # Main project folder
```

## 🤝 Credits & Acknowledgments

I wouldn't be spending hours developing Leaf without the passionate community of chess engine developers.

* **[Bullet contributors](https://github.com/jw1912/bullet/graphs/contributors?from=4%2F25%2F2026)** for providing excellent training platform.
* **[BBC Chess Engine Tutorials by Maksim Korzh](https://www.youtube.com/playlist?list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs)**
whose tutorials were an introduction to the chess programming world and which helped me developing
my previous engine from scratch.
* **[ChessProgramming Wiki Team](https://www.chessprogramming.org/Main_Page)** 
for the entire knowledge concentrated in one place.
* **As well as all the helpful developers from Engine Programming Discord Group.**

## 📜 License

This project is released under the GNU General Public License (GPL).
