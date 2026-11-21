# Leaf Utility Protocol

This document describes utility and development commands available in the **Leaf Utility Build**. These commands extend the standard UCI interface and are intended for testing, training data generation, SPSA tuning, and engine validation.

---

# General Syntax

Commands are entered as plain text lines:

```text
<command> [arguments...]
```

Arguments are whitespace-separated.

---

# Commands

## self_play

Starts a self-play tournament and collects training data.

### Syntax

```text
self_play <games> <threads> <output_dir> <error_log> [search limits]
```

### Arguments

| Argument        | Description                                    |
| --------------- | ---------------------------------------------- |
| `games`         | Total number of games to play                  |
| `threads`       | Number of worker threads                       |
| `output_dir`    | Directory where training files will be written |
| `error_log`     | Error log file                                 |
| `search limits` | Standard UCI search limits                     |

### Example

```text
self_play 50000 8 data/selfplay errors.log nodes 5000
```

```text
self_play 10000 4 data/selfplay errors.log depth 8
```

Every run stores its data in a generated sub-directory of `output_dir`:

```text
<output_dir>/selfplay_<mon><day>_<key>/session<N>/
```

The engines started for the tournament inherit the current process' `Hash` and
`SyzygyPath` settings, so set them before `self_play`.

### Supported Search Limits

Same format as the `go` command:

```text
depth <n>
nodes <n>
qnodes <n>
wtime <ms>
btime <ms>
winc <ms>
binc <ms>
```

---

## spsa

Starts SPSA parameter tuning.

### Syntax

```text
spsa <threads> <log_file> [hash_mb] [syzygy_path]
```

### Arguments

| Argument      | Description                                                        |
| ------------- | ------------------------------------------------------------------ |
| `threads`     | Number of tuning threads                                           |
| `log_file`    | SPSA tuning log output                                             |
| `hash_mb`     | Hash size forwarded to every tuning engine (default: `Hash` option) |
| `syzygy_path` | Tablebase directory forwarded to every tuning engine               |

Without the optional arguments the current process' `Hash` and `SyzygyPath`
settings are used.

### Example

```text
spsa 16 spsa.log
```

```text
spsa 16 spsa.log 64 assets/tb/Syzygy
```

---

## load_openings

Loads the opening book used by self-play.

### Syntax

```text
load_openings
```

### Example

```text
load_openings
```

---

## verify_session

Verifies integrity of collected training data.

The command scans all session directories and validates every stored position.

### Syntax

```text
verify_session <tournament_dir>
```

`tournament_dir` is one of the generated `selfplay_<mon><day>_<key>` run
directories (not the `data/selfplay` root).

### Example

```text
verify_session data/tournaments
```

### Validation Checks

For every position:

* Position structure is valid.
* Position is not a solved tablebase position.
* Side to move is not in check.
* File is fully readable without corruption.

---

## to_bullet_format

Converts collected selfplay data to bullet format **in place**.

### Syntax

```text
to_bullet_format [root]
```

Without `root` the whole `data/selfplay` database is converted; with `root` the
given directory tree is used instead. The conversion state is cached in a
`session.info` file inside every session directory, so sessions that already
are in bullet format are skipped. That makes it safe to call after every
collection batch.

### Example

```text
to_bullet_format
```

---

## shuffle_data

Merges the files of every bullet-format session, shuffles the records and
writes one file per session.

### Syntax

```text
shuffle_data [root]
```

Output goes to `<root>_shuffled/<session path>.tdf` (default:
`data/selfplay_shuffled`). Sessions must be converted with `to_bullet_format`
first; unconverted sessions are reported and skipped.

### Example

```text
shuffle_data
```

---

## view_positions

Displays positions stored inside an extended packed-position file.

### Syntax

```text
view_positions <file> <begin> <count>
```

### Arguments

| Argument | Description                    |
| -------- | ------------------------------ |
| `file`   | Packed position file           |
| `begin`  | Starting index                 |
| `count`  | Number of positions to display |

### Example

```text
view_positions positions.bin 100 10
```

Displays positions:

```text
100
101
...
109
```

---

# Test Commands

The following commands execute internal validation suites.

---

## test_perft

Runs the built-in perft test suite.

### Syntax

```text
test_perft
```

### Example

```text
test_perft
```

### Purpose

Verifies move generation correctness against known perft results.

---

## test_see

Runs Static Exchange Evaluation tests.

### Syntax

```text
test_see
```

---

## test_null_move

Runs null-move pruning validation tests.

### Syntax

```text
test_null_move
```

---

## test_pack

Runs packed-position serialization tests.

### Syntax

```text
test_pack
```

---

## test_pack_on

Runs packed-position verification on a file.

### Syntax

```text
test_pack_on <file>
```

### Example

```text
test_pack_on positions.bin
```

---

## test_extpack_on

Runs extended packed-position verification on a file.

### Syntax

```text
test_extpack_on <file>
```

### Example

```text
test_extpack_on training.bin
```

---

## test_ccr_one_hour

Runs the CCR one-hour test suite.

### Syntax

```text
test_ccr_one_hour
```

---

# UCI Compatibility

The utility build supports all standard engine commands:

```text
uci
ucinewgame
position
print
go
isready
export_net
rewrite_header
options
setoption
bench
quit
```

See `src/frontend/UCIEXTPROTOCOL.md` for detailed documentation of standard and extended UCI commands.

---

# Debug Build Commands

Available only when compiled with:

```cpp
_UCI_DEBUG_UTILS
```

---

## see

Computes Static Exchange Evaluation for a given capture square.

### Syntax

```text
see <origin> <destination>
```

### Example

```text
see e4 d5
```

---

## nneval

Evaluates a position using the loaded NNUE network.

### Syntax

```text
nneval <fen>
```

### Examples

```text
nneval startpos
```

```text
nneval rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
```

### Output

Network evaluation score in centipawns from the side-to-move perspective.
