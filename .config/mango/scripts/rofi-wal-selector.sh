#!/bin/sh

WALL_DIR="$HOME/Pictures/wallpaper"
CACHE_DIR="$HOME/.cache/temp-wal"
ROFI_THEME="$HOME/.config/rofi/wal-selector.rasi"

mkdir -p "$CACHE_DIR"

generate_list() {
    find "$WALL_DIR" -type f \
        \( \
            -iname "*.png" \
            -o -iname "*.jpg" \
            -o -iname "*.jpeg" \
            -o -iname "*.webp" \
            -o -iname "*.bmp" \
            -o -iname "*.gif" \
            -o -iname "*.tif" \
            -o -iname "*.tiff" \
        \) |
    while IFS= read -r file; do
        printf '%s\000icon\037%s\n' "$file" "$file"
    done
}

IMAGE=$(
    generate_list |
    rofi \
        -dmenu \
        -show-icons \
        -theme "$ROFI_THEME" \
        2>/dev/null
)

[ -n "$IMAGE" ] || exit 0

rm -f "$CACHE_DIR"/temp-wal.*

EXTENSION=${IMAGE##*.}
CACHED_IMAGE="$CACHE_DIR/temp-wal.$EXTENSION"

cp -- "$IMAGE" "$CACHED_IMAGE" || exit 1

wal -c || exit 1

awww img "$CACHED_IMAGE" || exit 1
wal -n -i "$CACHED_IMAGE" -o "$HOME/.config/wal/run-scripts.sh"
pkill waybar
waybar
