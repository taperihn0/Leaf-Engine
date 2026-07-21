import os
import sys
import shutil
import subprocess
import argparse
from pathlib import Path

# --- Tournament configurations ---
GAMES_COUNT = 8000
THREAD_COUNT = 6
TIME_CONTROL = "1+0.04"
CMAKE_PRESET = "final"
OPENING_BOOK = "assets/books/UHO_Lichess_4852_v1.epd"
OPENING_BOOK_FORMAT = "epd"
ENGINE_NAME = "Leaf"
SYZYGY_PATH = "/home/Szymek/Source/Leaf/assets/tb/Syzygy"
HALFED_GAMES_COUNT = GAMES_COUNT // 2
WORKSPACES_BASE_DIR_PATH = Path("workspaces/temporary/")
BINARY_PRESET_DIR = "Release" if CMAKE_PRESET == "final" or CMAKE_PRESET == "release" else "Debug"

curr_working_dir = Path(os.getcwd())

def run_command(cmd: list, cwd: Path = curr_working_dir, msg: str = ""):
    if msg:
        print(f"--> {msg}")

    process = subprocess.Popen(cmd, 
                               cwd=cwd, 
                               stdout=subprocess.PIPE, 
                               stderr=subprocess.PIPE, 
                               text=True)

    full_output = []

    for line in process.stdout:
        sys.stdout.write(line)
        sys.stdout.flush()
        full_output.append(line)

    process.wait()

    if process.returncode != 0:
        print(f"\n[ERROR], Command failed: {' '.join(cmd)}")
        sys.exit(1)

    return "".join(full_output)

def setup_and_build_binary(commit_hash: str, work_dir: Path) -> Path:    
    if work_dir.exists():
        shutil.rmtree(work_dir)
        
    run_command(["git", "worktree", "add", "--detach", str(work_dir), commit_hash], 
            msg=f"Creating git worktree for {commit_hash} in {work_dir}")

    build_dir = work_dir / "build"
    
    run_command(["cmake", 
                 f"--preset={CMAKE_PRESET}", 
                 "-S", str(work_dir), 
                 "-B", str(build_dir)], 
                 msg="Configuring CMake...")
    
    run_command(["cmake", 
                 "--build", str(build_dir), 
                 "--config", CMAKE_PRESET], 
                msg="Building binary...")

    potential_paths = [
        work_dir / "bin" / ENGINE_NAME / BINARY_PRESET_DIR / ENGINE_NAME,
        work_dir / "bin" / ENGINE_NAME / BINARY_PRESET_DIR / f"{ENGINE_NAME}.exe",
    ]

    for p in potential_paths:
        if p.is_file():
            return p

    print(f"[ERROR]: Binary '{ENGINE_NAME}' not found in build outputs.")
    sys.exit(1)

def run_tournament(bin_0_dir: Path, bin_1_dir: Path, 
                   version_0_id: str, version_1_id: str, 
                   version_0_name: str = "", version_1_name: str = ""):

    version_0_name = f"Leaf_{version_0_id}" if not version_0_name else version_0_name
    version_1_name = f"Leaf_{version_1_id}" if not version_1_name else version_1_name

    print(f"\n------------------- Starting tournament ---------------------")
    print(f"Version 0 ({version_0_id}): {bin_0_dir}")
    print(f"Version 1 ({version_1_id}): {bin_1_dir}")
    print("------------------------------------------------------------")

    cmd = [
        "cutechess-cli",

        # Engine 0 arguments
        "-engine", 
        f"cmd={bin_0_dir}", 
        f"name={version_0_name}", 
        f"initstr=setoption name SyzygyPath value {SYZYGY_PATH}",

        # Engine 1 arguments
        "-engine", 
        f"cmd={bin_1_dir}", 
        f"name={version_1_name}", 
        f"initstr=setoption name SyzygyPath value {SYZYGY_PATH}",

        # Configure common settings
        "-each", 
        "proto=uci", 
        f"tc={TIME_CONTROL}", 

        # Games and thread count
        "-rounds", "2",
        "-games", str(HALFED_GAMES_COUNT),
        "-draw", "movenumber=36", "movecount=8", "score=10", 
        "-concurrency", str(THREAD_COUNT),

        # Opening book mode
        "-openings",
        f"file={OPENING_BOOK}",
        f"format={OPENING_BOOK_FORMAT}", 
        f"order=random",
         "plies=8",
        f"policy=encounter",
        "-repeat"
    ]

    process = subprocess.Popen(cmd, 
                               stdout=sys.stdout, 
                               stderr=sys.stderr, 
                               text=True)
    process.wait()

def main():
    parser = argparse.ArgumentParser(description="Automated Leaf version tournament")

    parser.add_argument("engine_0_version", 
                        help="Baseline Leaf git version to verify: commit hash, branch, or tag (e.g. master)")
    parser.add_argument("engine_1_version", 
                        help="Leaf git version to verify against: commit hash, branch, or tag (e.g. feature-branch)")
    
    parser.add_argument("--name0", 
                        help="Optional custom name for the baseline engine version",
                        default=None)
    parser.add_argument("--name1", 
                        help="Optional custom name for the test engine version",
                        default=None)

    args = parser.parse_args()

    base_workspace_dir = WORKSPACES_BASE_DIR_PATH
    base_workspace = base_workspace_dir / f"version_{args.engine_0_version}"
    test_workspace = base_workspace_dir / f"version_{args.engine_1_version}"

    bin_base = None
    bin_test = None

    try:
        run_command(["git", "rev-parse", "--is-inside-work-tree"])
        
        bin_base = setup_and_build_binary(args.engine_0_version, base_workspace)
        bin_test = setup_and_build_binary(args.engine_1_version, test_workspace)

        run_tournament(bin_base, bin_test, 
                       args.engine_0_version, 
                       args.engine_1_version, 
                       args.name0, 
                       args.name1)
    finally:
        if base_workspace.exists():
            run_command(["git", "worktree", "remove", "--force", str(base_workspace)])

        if test_workspace.exists():
            run_command(["git", "worktree", "remove", "--force", str(test_workspace)])
            
        if base_workspace_dir.exists() and \
           not os.listdir(base_workspace_dir):
            base_workspace_dir.rmdir()

        print("Worktree cleaned.")

if __name__ == "__main__":
    main()
