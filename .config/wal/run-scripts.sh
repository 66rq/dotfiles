#!/bin/bash

for script in "$HOME"/.config/wal/scripts/*.sh; do
    [[ -x "$script" ]] || continue

    "$script" || printf 'Failed: %s\n' "$script" >&2
done
