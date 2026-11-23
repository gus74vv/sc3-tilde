#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""4.x — coexistencia con el IDE de SuperCollider (gate del puerto AUTO).

Ocupa 127.0.0.1:57110 igual que el asio de scsynth (SIN SO_REUSEADDR) y corre
engine_test con el puerto default (0 = auto). El boot tiene que quedar ready en
otro puerto y sclang tiene que apuntar a ese mismo puerto.

Uso: python3 tests/coexist_check.py [./build/engine_test]
"""
import socket
import subprocess
import sys

BIN = sys.argv[1] if len(sys.argv) > 1 else "./build/engine_test"
PORT = 57110

blocker = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
try:
    blocker.bind(("127.0.0.1", PORT))   # sin SO_REUSEADDR: como scsynth
except OSError as e:
    sys.exit(f"[SKIP] {PORT} was already busy in another way: {e}")

try:
    r = subprocess.run([BIN], capture_output=True, text=True)
    out = r.stdout + r.stderr
    for line in out.splitlines():
        if "4.x" in line or "[FAIL" in line:
            print(line)
    ok = (r.returncode == 0
          and "[FAIL" not in out
          and f"World en puerto {PORT}" not in out
          and "4.x: sclang apunta al World" in out)
    print(f"== coexistencia con {PORT} ocupado: {'OK' if ok else 'FAIL'} "
          f"(exit={r.returncode}, [OK ]={out.count('[OK ]')}) ==")
    sys.exit(0 if ok else 1)
finally:
    blocker.close()
