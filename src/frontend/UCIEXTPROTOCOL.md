# UCIEXTPROTOCOL.md

## Introduction

This engine implements the standard Universal Chess Interface (UCI) protocol together with several custom extensions intended for development, testing, self-play data generation, neural network management and debugging.

All commands are case-sensitive.

---

# Standard UCI Commands

## `uci`

Initializes UCI mode.

### Syntax

```text
uci
```

### Response

```text
id name <engine name>
id author <author>
uciok
```

### Example

```text
uci
```

---

## `isready`

Checks whether the engine is ready to receive commands.

### Syntax

```text
isready
```

### Response

```text
readyok
```

### Example

```text
isready
```

---

## `ucinewgame`

Clears internal search state and prepares the engine for a new game.

### Syntax

```text
ucinewgame
```

### Notes

This command clears:

* search history
* transposition-related game state
* repetition tracking

### Example

```text
ucinewgame
```

---

## `position`

Loads a chess position.

### Syntax

```text
position startpos [moves ...]
```

```text
position fen <fen-string> [moves ...]
```

### Supported Keywords

#### Start Position

```text
position startpos
```

#### FEN Position

```text
position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
```

#### Apply Moves

```text
position startpos moves e2e4 e7e5 g1f3
```

### Custom Position

The engine additionally supports:

```text
position kiwipete
```

which loads the classical Kiwipete test position.

---

## `go`

Starts searching.

### Syntax

```text
go [arguments]
```

### Supported Arguments

| Argument | Description                              |
| -------- | ---------------------------------------- |
| depth    | Maximum search depth                     |
| nodes    | Node search limit                        |
| qnodes   | Quiescence node limit (custom extension) |
| wtime    | White remaining time (ms)                |
| btime    | Black remaining time (ms)                |
| winc     | White increment (ms)                     |
| binc     | Black increment (ms)                     |

### Examples

Depth limited search:

```text
go depth 10
```

Node limited search:

```text
go nodes 1000000
```

Time controlled search:

```text
go wtime 300000 btime 300000 winc 2000 binc 2000
```

Combined constraints:

```text
go depth 12 nodes 500000
```

### Custom Extension: Quiescence Node Limit

```text
go qnodes 100000
```

Limits only quiescence search nodes.

---

## `quit`

Terminates the engine.

### Syntax

```text
quit
```

---

# Engine Utility Commands

## `print`

Prints the current position.

### Syntax

```text
print
```

### Example

```text
position startpos moves e2e4
print
```

---

## `options`

Displays all available engine options.

### Syntax

```text
options
```

### Example

```text
options
```

---

## `setoption`

Changes engine configuration options.

### Syntax

```text
setoption name <option> value <value>
```

---

### Hash Size

```text
setoption name Hash value 64
```

Sets transposition table size in megabytes.

---

### Clear Hash

```text
setoption name Clear Hash
```

Clears the transposition table.

---

### Syzygy Tablebases

```text
setoption name SyzygyPath value /path/to/syzygy
```

Loads Syzygy tablebases.

Multiple paths are currently not supported.

---

### Neural Network

```text
setoption name NeuralNetPath value network.nnue
```

Loads a neural network from file.

---

### Tunable Parameters

When tuning support is enabled (`_ENABLE_TUNING`), every tunable search parameter may be modified dynamically:

```text
setoption name NullReduction value 20
```

```text
setoption name AspirationFirstWindow value 45
```

```text
setoption name LmrDepth value 3
```

The complete list can be displayed with:

```text
options
```

---

# Neural Network Commands

## `export_net`

Loads a neural network file.

### Syntax

```text
export_net <path>
```

### Special Value

```text
export_net default
```

Loads the engine's default network.

### Examples

```text
export_net default
```

```text
export_net net.nnue
```

---

## `rewrite_header`

Rewrites a network file using the engine's current NNUE header format.

### Syntax

```text
rewrite_header <input_file> <output_file>
```

### Example

```text
rewrite_header old.nnue converted.nnue
```

Useful when migrating network files between engine versions.

---

# Benchmark Command

## `bench`

Runs the internal benchmark suite.

### Syntax

```text
bench [depth]
```

### Examples

Default benchmark depth:

```text
bench
```

Custom depth:

```text
bench 8
```

### Output

```text
Searched <nodes> nodes in <time>s
```

---

# Debug Commands

The following commands are available only when the engine is compiled with:

```cpp
_UCI_DEBUG_UTILS
```

---

## `see`

Runs Static Exchange Evaluation (SEE).

### Syntax

```text
see <from-square> <to-square>
```

### Example

```text
see e4 d5
```

### Output

Returns the material gain/loss score for the exchange sequence.

---

## `nneval`

Evaluates a position using the loaded NNUE network.

### Syntax

```text
nneval <fen>
```

or

```text
nneval startpos
```

### Examples

```text
nneval startpos
```

```text
nneval rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
```

### Output

Returns the NNUE evaluation score in centipawns.

---

# Self-Play Mode

The executable may be started with:

```text
--self-play
```

This enables binary I/O support required by the self-play data collection framework.

---

# Notes

* All move strings use long algebraic coordinate notation:

```text
e2e4
g1f3
e7e8q
```

* Invalid moves are rejected.
* Invalid FEN strings may result in undefined behavior.
* All times are expressed in milliseconds.
* Search limits may be combined.
* Custom commands are intended primarily for engine development and tuning workflows.
