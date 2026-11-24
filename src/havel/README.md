# Havel

A canvas-based window manager for X11.

## About

Havel is an X11 window manager that transforms your desktop into a zoomable, pannable canvas, allowing you to navigate and arrange windows freely.

> [!NOTE]
> Havel has built-in compositing and does not support external compositors such as Picom.

## Dependencies

Void Linux

```bash
sudo xbps-install -S base-devel make gcc libX11-devel libXrender-devel libXcomposite-devel libXdamage-devel libXfixes-devel libXi-devel libXpresent-devel libXrandr-devel
```
Arch Linux

```bash
sudo pacman -Syu base-devel libx11 libxrender libxcomposite libxdamage libxfixes libxi libxpresent libxrandr make gcc
```


## Installation

Clone the repository:
```bash
git clone https://codeberg.org/333s1/havel.git 
cd havel
```

Build and install:
```sh
doas make install clean
```

For reload config:
```sh
doas make reload
```

You can use sudo instead of doas if preferred.

Usage
Display Manager: Select Havel from your session list.
startx: Add the following line to your ~/.xinitrc:

```bash
#!/bin/sh

exec havel
```
