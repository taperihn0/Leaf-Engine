#!/bin/bash

log_msg() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

if [ "$#" -ne 5 ]; then
    echo "Usage: $0 <executable_path> <total_games> <games_per_session> <thread_count> <nodes>"
    echo "Example: $0 ./bin/Utils/Release/LeafUtils 100000 4000 2 8000"
    exit 1
fi

EXECUTABLE=$1
TOTAL_GAMES=$2
GAMES_PER_SESSION=$3
THREAD_COUNT=$4
NODES=$5

DATE_STR=$(date +%b%d | tr '[:upper:]' '[:lower:]')
BASE_DIR="misc/selfplay_$DATE_STR"

if [ -d "$BASE_DIR" ]; then
    suffix_code=97
    while true; do
        suffix=$(printf "\\$(printf '%03o' $suffix_code)")
        NEW_DIR="${BASE_DIR}_${suffix}"
        if [ ! -d "$NEW_DIR" ]; then
            BASE_DIR="$NEW_DIR"
            break
        fi
        ((suffix_code++))
        if [ $suffix_code -gt 122 ]; then
            log_msg "ERROR: Too many daily sessions! Manual cleanup required."
            exit 1
        fi
    done
fi

if [ ! -f "$EXECUTABLE" ]; then
    log_msg "ERROR: Executable '$EXECUTABLE' not found."
    exit 1
fi

log_msg "Batch tournament"
echo "Binary:    $EXECUTABLE"
echo "Target:    $TOTAL_GAMES games"
echo "Batch:     $GAMES_PER_SESSION"
echo "Threads:   $THREAD_COUNT"
echo "Nodes:     $NODES"
echo "Storage:    $BASE_DIR/"
echo "----------------------------------------------------"

GAMES_PLAYED=0
SESSION_NUM=1

while [ $GAMES_PLAYED -lt $TOTAL_GAMES ]; do
    SESSION_DIR="$BASE_DIR/session$SESSION_NUM"
    ERR_FILE="$SESSION_DIR/err"

    mkdir -p "$SESSION_DIR"
    : > "$ERR_FILE"

    log_msg "Launching Session $SESSION_NUM..."

    $EXECUTABLE <<EOF
self_play $GAMES_PER_SESSION $THREAD_COUNT $SESSION_DIR $ERR_FILE nodes $NODES
EOF

    EXIT_CODE=$?
    if [ $EXIT_CODE -ne 0 ]; then
        log_msg "Crash (Code $EXIT_CODE) in Session $SESSION_NUM."
        log_msg "Restarting in 5s..."
        sleep 5 
    fi

    GAMES_PLAYED=$((GAMES_PLAYED + GAMES_PER_SESSION))
    ((SESSION_NUM++))
    
    log_msg "Progress: $GAMES_PLAYED / $TOTAL_GAMES games completed."
    echo "----------------------------------------------------"
done

log_msg "Tournament Complete. Data in: $BASE_DIR"
