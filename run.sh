#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BINARY="${BUILD_DIR}/VirtualPet/VirtualPet"

echo "============================================================"
echo "      Virtual Being Companion — Developer Runner"
echo "============================================================"

# 1. Dependency Pre-flight Check
echo "[run.sh] Checking development dependencies..."

if ! command -v cmake >/dev/null 2>&1; then
    echo "[run.sh] ERROR: cmake is not installed. Please install cmake."
    exit 1
fi

if ! command -v pkg-config >/dev/null 2>&1; then
    echo "[run.sh] ERROR: pkg-config is not installed. Please install pkg-config."
    exit 1
fi

# Check for GTK3
if pkg-config --exists gtk+-3.0; then
    GTK_VER="$(pkg-config --modversion gtk+-3.0)"
    echo "[run.sh] Found GTK+ 3.0 (version ${GTK_VER})."
    echo "[run.sh] Native GTK desktop overlay, chat panel, and system tray are ready."
else
    echo "[run.sh] WARNING: gtk+-3.0 not found!"
    echo "[run.sh] On Fedora/RHEL: sudo dnf install gtk3-devel"
    echo "[run.sh] On Ubuntu/Debian: sudo apt install libgtk-3-dev"
    exit 1
fi

# 2. Build the project
echo "[run.sh] Configuring & building VirtualPet..."
cmake -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Debug -S "${SCRIPT_DIR}"
cmake --build "${BUILD_DIR}" --parallel "$(nproc 2>/dev/null || echo 2)"

if [ ! -f "${BINARY}" ]; then
    echo "[run.sh] ERROR: Binary ${BINARY} was not produced."
    exit 1
fi

echo "[run.sh] Build complete."
echo "------------------------------------------------------------"
echo "Starting VirtualPet in standalone dev mode..."
echo "• Left-click to pet or drag"
echo "• Double-click or click speech bubble to chat with Astra"
echo "• Right-click for menu (Headpat, Boba, Plushie, Study, Status)"
echo "• Press Ctrl+C in this terminal to stop at any time"
echo "• NO startup files or autostart entries are registered"
echo "------------------------------------------------------------"

# 3. Run from VirtualPet directory so assets/ and config/ resolve
cd "${SCRIPT_DIR}/VirtualPet"

# If running under Hyprland, ensure companion windows float and stay above full-screen windows
if command -v hyprctl >/dev/null 2>&1 && [ -n "${HYPRLAND_INSTANCE_SIGNATURE:-}" ]; then
    echo "[run.sh] Hyprland detected — registering overlay rules..."
    hyprctl keyword windowrulev2 "float, class:^(VirtualPet)$" >/dev/null 2>&1 || true
    hyprctl keyword windowrulev2 "pin, class:^(VirtualPet)$" >/dev/null 2>&1 || true
    hyprctl keyword windowrulev2 "noborder, class:^(VirtualPet)$" >/dev/null 2>&1 || true
    hyprctl keyword windowrulev2 "noshadow, class:^(VirtualPet)$" >/dev/null 2>&1 || true
fi

# Run binary in background and capture PID
"${BINARY}" "$@" &
PET_PID=$!

# Trap SIGINT and SIGTERM to kill process cleanly on exit
cleanup() {
    trap - INT TERM
    echo ""
    echo "[run.sh] Stopping VirtualPet (PID ${PET_PID})..."
    if kill -0 "${PET_PID}" 2>/dev/null; then
        kill -TERM "${PET_PID}" 2>/dev/null || true
        wait "${PET_PID}" 2>/dev/null || true
    fi
    echo "[run.sh] VirtualPet stopped cleanly. Have a great day!"
}

trap cleanup INT TERM

# Wait for process to terminate
wait "${PET_PID}" 2>/dev/null || true
trap - INT TERM
echo "[run.sh] VirtualPet finished. Have a great day!"
