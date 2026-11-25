# dwm


## Dependencies

To build you need a C compiler and `make`, `git`, and the `Xlib`, `Xft`,
`Xinerama`, `freetype` and `fontconfig` header files.

Arch Linux:

```bash
sudo pacman -S --needed base-devel git libx11 libxft libxinerama freetype2 fontconfig
```

Void Linux:

```bash
sudo xbps-install -S base-devel git libX11-devel libXft-devel libXinerama-devel freetype-devel fontconfig-devel
```

## Clone and Install


```bash
git clone https://codeberg.org/333s1/dwm.git && cd dwm && make && sudo make install clean
```

## Running dwm

You will need the `startx` utility installed (`xorg-xinit` on Arch, `xinit` on
Void). Your `.xinitrc` should be something like this:

```bash
#!/bin/sh

exec dwm
```

To show status info in the bar (needs `xsetroot`: `xorg-xsetroot` on Arch,
`xsetroot` on Void):

```bash
while xsetroot -name "`date` `uptime | sed 's/.*,//'`"
do
	sleep 1
done &
exec dwm
```

## Configuration

| Option | What it does |
| --- | --- |
| `showbar` / `topbar` | show the bar at all / put it on top (1) or bottom (0) |
| `uppad` `bottompad` `leftpad` `rightpad` | free gap around the bar, in pixels (see below) |
| `showclienttitle` | 1 shows the title of the focused window in the bar, 0 hides it |
| `titlecolor_onbar` | background color of that title (text uses `sel_fg`) |
| `hide_vacant_tags` | 1 hides tags with no windows, 0 shows all nine tags |
| `borderpx` / `gappx` / `snap` | border width, gap between windows, snap distance |
| `fonts` | bar font (fontconfig name) |
| `tags` / `rules` | tag labels / per-application rules |

The four pads work like margins around the bar, and windows never overlap the
bar or its gaps:

- `leftpad` / `rightpad`: gap between the bar and the left / right screen edge.
- `uppad`: gap above the bar (the screen edge if the bar is on top, the gap
  between the bar and the windows if it is at the bottom).
- `bottompad`: gap below the bar (the gap between the bar and the windows if the
  bar is on top, the screen edge if it is at the bottom).

Colors are `norm_*` (normal) and `sel_*` (selected) in `dwm-colors.h`. The title
color is kept in `config.def.h` on purpose, so it keeps working if you replace or
regenerate the palette file.
