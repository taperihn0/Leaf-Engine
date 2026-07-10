import os
import platform
import subprocess
import sys
from pathlib import Path

def run_rust_file(source_file_path: str, program_args: list[str]):
    source_path = Path(source_file_path)

    if not source_path.exists():
        print(f"Error: File {source_path} does not exist.")
        return

    exec_name = source_path.stem
    if platform.system() == "Windows":
        exec_name += ".exe"
    exec_path = source_path.parent / exec_name

    try:
        print(f"Compiling {source_path.name}...")
        subprocess.run(["rustc", "-C", "opt-level=3", str(source_path), "-o", str(exec_path)], check=True)

        run_cmd = [str(exec_path.resolve())] + program_args
        print(f"Executing: {' '.join(run_cmd)}")

        subprocess.run(run_cmd, check=True)

    except subprocess.CalledProcessError:
        print("Failure during compilation or execution.")

    finally:
        if exec_path.exists():
            try:
                exec_path.unlink()
            except OSError as e:
                print(f"Failed to cleanup file {exec_path.name}: {e}")

        if platform.system() == "Windows":
            pdb_path = exec_path.with_suffix(".pdb")
            if pdb_path.exists():
                try:
                    pdb_path.unlink()
                except OSError:
                    pass


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python scripts/BulletFormat.py <path_to_fen_file>")
    else:
        args_for_rust = [sys.argv[1]]
        rust_file = "scripts/TrainDat2Bullet.rs"
        run_rust_file(rust_file, args_for_rust)
