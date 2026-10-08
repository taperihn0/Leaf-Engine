import sys
import os
import json
import time
import subprocess
from pathlib import Path
from datetime import datetime

PROJECT_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONFIG_PATH = PROJECT_ROOT / "configs" / "DataConfig.json"

def _load_config(path: Path) -> dict:
    defaults = {
        "executable": "./bin/Utils/Release/LeafUtils",
        "total_sessions": 10,
        "games_per_session": 4000,
        "thread_count": 2,
        "nodes": 8000,
        "syzygy_path": "Leaf-Engine/src/assets/tb/Syzygy",
    }
    cfg = dict(defaults)
    if path.is_file():
        with open(path, encoding="utf-8") as f:
            parsed = json.load(f)
        cfg.update({k: v for k, v in parsed.items() if k in defaults})
    return cfg

def log_msg(msg: str):
    time_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{time_str}] {msg}")

def main():
    cfg = _load_config(DEFAULT_CONFIG_PATH)

    if len(sys.argv) == 1:
        executable = os.path.normpath(cfg["executable"])
        try:
            total_sessions = int(cfg["total_sessions"])
            games_per_session = int(cfg["games_per_session"])
            thread_count = int(cfg["thread_count"])
            nodes = int(cfg["nodes"])
        except ValueError:
            log_msg("ERROR: Config numeric values must be integers.")
            sys.exit(1)
        syzygy_path = str(cfg["syzygy_path"])
    elif len(sys.argv) == 6:
        executable = os.path.normpath(sys.argv[1])
        try:
            total_sessions = int(sys.argv[2])
            games_per_session = int(sys.argv[3])
            thread_count = int(sys.argv[4])
            nodes = int(sys.argv[5])
        except ValueError:
            log_msg("ERROR: Numeric arguments must be integers.")
            sys.exit(1)
        syzygy_path = "<empty>"
    elif len(sys.argv) == 7:
        executable = os.path.normpath(sys.argv[1])
        try:
            total_sessions = int(sys.argv[2])
            games_per_session = int(sys.argv[3])
            thread_count = int(sys.argv[4])
            nodes = int(sys.argv[5])
        except ValueError:
            log_msg("ERROR: Numeric arguments must be integers.")
            sys.exit(1)
        syzygy_path = str(sys.argv[6])
    else:
        print(f"Usage: {sys.argv[0]} <executable_path> <total_sessions> <games_per_session> <thread_count> <nodes> <syzygy_path(optional)>")
        print(f"Example: {sys.argv[0]} ./bin/Utils/Release/LeafUtils 10 4000 2 8000 Leaf-Engine/src/assets/tb/Syzygy")
        sys.exit(1)

    if not os.path.isfile(executable):
        log_msg(f"ERROR: Executable '{executable}' not found.")
        sys.exit(1)

    date_str = datetime.now().strftime("%b%d").lower()
    base_dir = os.path.join("data/selfplay", f"selfplay_{date_str}")

    if os.path.isdir(base_dir):
        suffix_code = 97
        while True:
            suffix = chr(suffix_code)
            new_dir = f"{base_dir}_{suffix}"
            if not os.path.isdir(new_dir):
                base_dir = new_dir
                break
            suffix_code += 1
            if suffix_code > 122:
                log_msg("ERROR: Too many daily sessions! Manual cleanup required.")
                sys.exit(1)

    log_msg("Batch tournament")
    print(f"Binary:    {executable}")
    print(f"Sessions:  {total_sessions}")
    print(f"Batch:     {games_per_session}")
    print(f"Threads:   {thread_count}")
    print(f"Nodes:     {nodes}")
    print(f"Storage:   {base_dir}{os.sep}")
    print(f"Syzygy:    {syzygy_path}")
    print("----------------------------------------------------")

    session_num = 1

    while session_num <= total_sessions:
        session_dir = os.path.normpath(os.path.join(base_dir, f"session{session_num}"))
        err_file = os.path.normpath(os.path.join(session_dir, "err"))

        os.makedirs(session_dir, exist_ok=True)
        
        with open(err_file, "w", encoding="utf-8") as f:
            f.truncate(0)

        log_msg(f"Launching Session {session_num}...")

        if (syzygy_path != "<empty>"):
            input_cmds = f"setoption name SyzygyPath value {syzygy_path}\n"

        input_cmds += f"self_play {games_per_session} {thread_count} {session_dir} nodes {nodes}\n"

        try:
            process = subprocess.Popen(
                [executable],
                stdin=subprocess.PIPE,
                stdout=sys.stdout,
                stderr=sys.stderr,
                text=True
            )

            process.communicate(input=input_cmds)
            exit_code = process.returncode

        except Exception as e:
            log_msg(f"Process execution failed: {e}")
            exit_code = -1

        if exit_code != 0:
            log_msg(f"Crash (Code {exit_code}) in Session {session_num}.")

            err_file = os.path.normpath(os.path.join(session_dir, "err"))
            time_now = datetime.now().strftime("%H:%M:%S")

            with open(err_file, "a", encoding="utf-8") as f:
                f.write(f"[{time_now}] SCRIPT: Process exited with code {exit_code}\n")
            
            log_msg("Restarting in 5s...")
            time.sleep(5)
            continue

        log_msg(f"Progress: {session_num} / {total_sessions} sessions completed.")
        print("----------------------------------------------------")
        
        session_num += 1

    log_msg("Tournament finished.")

if __name__ == "__main__":
    main()
    
