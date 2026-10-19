#!/bin/sh
# Builds GoWR43-Patcher.exe and GoWR43-Undo.exe into bin/
# Needs Zig 0.17.0 on PATH, or: pip install ziglang==0.17.0
set -e
cd "$(dirname "$0")"
mkdir -p bin

if command -v zig >/dev/null 2>&1; then ZIG="zig"; else ZIG="python3 -m ziglang"; fi

$ZIG cc -target x86_64-windows-gnu -O2 -s src/gowr43patch.c -lshell32 -ladvapi32 -o bin/GoWR43-Patcher.exe
$ZIG cc -target x86_64-windows-gnu -O2 -s -DUNDO_BUILD src/gowr43patch.c -lshell32 -ladvapi32 -o bin/GoWR43-Undo.exe

echo "Built:"
sha256sum bin/*.exe 2>/dev/null || shasum -a 256 bin/*.exe
