#!/bin/sh 

cp "$HOME/.cache/wal/Xresources" "$HOME/.Xresources"
xrdb -merge -quiet "$HOME/.Xresources"
