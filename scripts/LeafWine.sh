#!/usr/bin/env bash

# ==============================================================================
# Script to run a MinGW-w64 executable on Wine with Win32 Large Pages support
# ==============================================================================

set -e

# Configuration: Adjust these variables if needed
EXE_PATH="${1:-./my_app.exe}"   # Pass binary as argument or set default here
HUGEPAGES_NEEDED=256            # Number of 2MB HugePages (512 = 1024 MB / 1 GB)
WINESERVER_BIN="$(which wineserver 2>/dev/null || true)"

echo "=== Wine Large Pages Runner ==="

# 1. Check if the binary exists
if [ ! -f "$EXE_PATH" ]; then
    echo "Error: Executable '$EXE_PATH' not found!"
    echo "Usage: $0 /path/to/binary.exe"
    exit 1
fi

# 2. Check and allocate Linux kernel HugePages if necessary
CURRENT_PAGES=$(sysctl -n vm.nr_hugepages 2>/dev/null || echo 0)
echo "[1/4] Checking kernel HugePages (Current: ${CURRENT_PAGES}, Needed: ${HUGEPAGES_NEEDED})..."

if [ "$CURRENT_PAGES" -lt "$HUGEPAGES_NEEDED" ]; then
    echo "      Increasing vm.nr_hugepages to ${HUGEPAGES_NEEDED}..."``
    if [ "$EUID" -ne 0 ]; then
        echo "      Root privileges required to set sysctl. Requesting sudo..."
        sudo sysctl -w vm.nr_hugepages="$HUGEPAGES_NEEDED"
    else
        sysctl -w vm.nr_hugepages="$HUGEPAGES_NEEDED"
    fi
fi

# 3. Raise process locked memory limit (memlock)
echo "[2/4] Setting ulimit -l (memlock) to unlimited..."
ulimit -l unlimited 2>/dev/null || {
    echo "      Warning: Failed to set ulimit -l unlimited."
    echo "      Ensure '/etc/security/limits.conf' contains: * hard memlock unlimited"
}

# 4. Grant CAP_IPC_LOCK to wineserver if needed
echo "[3/4] Checking wineserver capabilities..."
if [ -n "$WINESERVER_BIN" ]; then
    # Resolve any symlinks to get the real binary
    REAL_WINESERVER="$(readlink -f "$WINESERVER_BIN")"
    
    HAS_CAP=$(getcap "$REAL_WINESERVER" 2>/dev/null | grep -i "cap_ipc_lock" || true)
    if [ -z "$HAS_CAP" ]; then
        echo "      Granting CAP_IPC_LOCK to $REAL_WINESERVER..."
        if [ "$EUID" -ne 0 ]; then
            sudo setcap cap_ipc_lock=+ep "$REAL_WINESERVER"
        else
            setcap cap_ipc_lock=+ep "$REAL_WINESERVER"
        fi
    else
        echo "      wineserver ($REAL_WINESERVER) already has CAP_IPC_LOCK capability."
    fi
else
    echo "      Warning: wineserver binary not found in PATH."
fi

# 5. Launch the application via Wine
echo "[4/4] Launching '$EXE_PATH' via Wine..."
echo "=============================================================================="

# Optional debug output to verify memory allocation in Wine output
# export WINEDEBUG="+module,+virtual" 

wine "$EXE_PATH" "${@:2}"
