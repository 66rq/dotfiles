#!/usr/bin/env python3

import json
import subprocess


def run_playerctl(*args):
    try:
        result = subprocess.run(
            ["playerctl", *args],
            capture_output=True,
            text=True,
            check=True,
            timeout=2,
        )
        return result.stdout.strip()
    except (OSError, subprocess.SubprocessError):
        return ""


metadata = run_playerctl(
    "metadata",
    "--format",
    "{{artist}}|{{title}}",
)

if not metadata:
    output = {"text": "", "class": "empty", "tooltip": ""}
else:
    artist, _, title = metadata.partition("|")
    artist = artist.strip()
    title = title.strip()
    track = f"{artist} — {title}" if artist and title else title or artist
    status = run_playerctl("status").lower() or "paused"

    if track:
        output = {
            "text": f"♫ {track}",
            "tooltip": track,
            "class": status,
        }
    else:
        output = {"text": "", "class": "empty", "tooltip": ""}

print(json.dumps(output, ensure_ascii=False, separators=(",", ":")))
