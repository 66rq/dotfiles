#!/usr/bin/env python3

import json
import subprocess
import time


def emit(title):
    title = title or ""
    print(
        json.dumps(
            {
                "text": title,
                "tooltip": title,
                "class": "focused" if title else "empty",
            },
            ensure_ascii=False,
            separators=(",", ":"),
        ),
        flush=True,
    )


while True:
    watcher = None
    try:
        watcher = subprocess.Popen(
            ["mmsg", "watch", "focusing-client"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            encoding="utf-8",
            errors="replace",
            bufsize=1,
        )

        if watcher.stdout is None:
            raise RuntimeError("Unable to read Mango IPC stream")

        for line in watcher.stdout:
            try:
                client = json.loads(line)
            except json.JSONDecodeError:
                continue

            if isinstance(client, dict):
                emit(client.get("title", ""))

    except (OSError, subprocess.SubprocessError, RuntimeError):
        emit("")
    finally:
        if watcher is not None and watcher.poll() is None:
            watcher.terminate()
            try:
                watcher.wait(timeout=1)
            except subprocess.TimeoutExpired:
                watcher.kill()
                watcher.wait()

    time.sleep(0.5)
