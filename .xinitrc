#!/bin/sh

export XCURSOR_THEME=white-cursor
export XCURSOR_SIZE=11

# xsetroot -solid "#000000"
xset s off
xset -dpms
xset s noblank
setxkbmap -layout "us,ara" -option "grp:alt_shift_toggle"
xset r rate 250 40
xmodmap "$HOME/.Xmodmap"
xrdb -merge "$HOME/.Xresources"
#sxhkd &
xclip &
xrandr --output LVDS --mode 1600x900 --output VGA-0 --mode 1680x1050 --primary --left-of LVDS
xwallpaper --zoom "$(find $HOME/.cache/temp-wal -name 'temp-wal*' -type f | head -n 1)" &

pipewire &
#sxbar &
status &
exec dbus-run-session havel
