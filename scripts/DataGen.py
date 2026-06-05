import sys
import os
import time
import subprocess
from datetime import datetime

def log_msg(msg: str):
    time_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{time_str}] {msg}")

def main():
    if len(sys.argv) != 6 and len(sys.argv) != 7:
        print(f"Usage: {sys.argv[0]} <executable_path> <total_sessions> <games_per_session> <thread_count> <nodes> <syzygy_path(optional)>")
        print(f"Example: {sys.argv[0]} ./bin/Utils/Release/LeafUtils 10 4000 2 8000 Leaf-Engine/src/assets/tb/Syzygy")
        sys.exit(1)

    executable = os.path.normpath(sys.argv[1])
    try:
        total_sessions = int(sys.argv[2])
        games_per_session = int(sys.argv[3])
        thread_count = int(sys.argv[4])
        nodes = int(sys.argv[5])
    except ValueError:
        log_msg("ERROR: Numeric arguments must be integers.")
        sys.exit(1)

    syzygy_path = str(sys.argv[6]) if len(sys.argv) == 7 else "<empty>"

    if not os.path.isfile(executable):
        log_msg(f"ERROR: Executable '{executable}' not found.")
        sys.exit(1)

    date_str = datetime.now().strftime("%b%d").lower()
    base_dir = os.path.join("misc", f"selfplay_{date_str}")

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

        safe_session_dir = session_dir.replace(os.sep, '/')
        safe_err_file = err_file.replace(os.sep, '/')
        
        input_cmds = f"self_play {games_per_session} {thread_count} {safe_session_dir} {safe_err_file} nodes {nodes}\n"
        if (syzygy_path != "<empty>"):
            input_cmds += f"setoption name SyzygyPath value {syzygy_path}\n"

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
    
