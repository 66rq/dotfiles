#!/usr/bin/env python3

import fcntl
import json
import os
import secrets
import subprocess
import sys
import time


runtime_dir = os.environ.get("XDG_RUNTIME_DIR", "/tmp")
state_file = os.path.join(runtime_dir, f"waybar-media-{os.getuid()}.json")
lock_file = state_file + ".lock"
double_click_window = 0.32


def player_action(action):
    try:
        subprocess.run(
            ["playerctl", action],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
            timeout=3,
        )
    except (OSError, subprocess.SubprocessError):
        pass


def load_state():
    try:
        with open(state_file, "r", encoding="utf-8") as stream:
            value = json.load(stream)
            return value if isinstance(value, dict) else {}
    except (OSError, json.JSONDecodeError):
        return {}


def save_state(state):
    temporary = f"{state_file}.{os.getpid()}"
    with open(temporary, "w", encoding="utf-8") as stream:
        json.dump(state, stream)
    os.replace(temporary, state_file)


def deferred_next(token):
    time.sleep(double_click_window)
    should_next = False

    with open(lock_file, "a+", encoding="utf-8") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        state = load_state()
        if state.get("token") == token:
            save_state({})
            should_next = True

    if should_next:
        player_action("next")


def right_click():
    action = None
    token = None

    with open(lock_file, "a+", encoding="utf-8") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        state = load_state()
        elapsed = time.monotonic() - state.get("time", 0)
        pending = state.get("token")

        if pending and elapsed <= double_click_window:
            save_state({})
            action = "previous"
        else:
            token = secrets.token_hex(8)
            save_state({"token": token, "time": time.monotonic()})

    if action:
        player_action(action)

    if token:
        subprocess.Popen(
            [sys.executable, os.path.abspath(__file__), "deferred", token],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            start_new_session=True,
        )


mode = sys.argv[1] if len(sys.argv) > 1 else ""

if mode == "toggle":
    player_action("play-pause")
elif mode == "right":
    right_click()
elif mode == "deferred" and len(sys.argv) > 2:
    deferred_next(sys.argv[2])
