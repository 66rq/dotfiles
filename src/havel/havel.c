/*
 * Havel — X11 canvas window manager.
 * Keybindings and visual settings live in config.h.
 */

#define _DEFAULT_SOURCE
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xpresent.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xrender.h>



typedef struct Havel Havel;
typedef union {
    int i;
    unsigned int ui;
    float f;
    const void *v;
} Arg;
typedef void (*KeyFunc)(Havel *, const Arg *);
typedef struct {
    unsigned int mod;
    KeySym       keysym;
    KeyFunc      func;
    Arg          arg;
} Key;
static void spawn(Havel *, const Arg *);
static void kill_window(Havel *, const Arg *);
static void reload_havel(Havel *, const Arg *);
static void lookat_focused_window(Havel *, const Arg *);
static void fullscreen_toggle(Havel *, const Arg *);
static void jump_to_window(Havel *, const Arg *);
static void switch_workspace(Havel *, const Arg *);
static void move_to_workspace(Havel *, const Arg *);
static void zoom_in(Havel *, const Arg *);
static void zoom_out(Havel *, const Arg *);
#include "config.h"
#define NBINDINGS (int)(sizeof(keys) / sizeof(keys[0]))

typedef struct {
    Window window;
    Visual *visual;
    Damage damage;
    double cx, cy;
    int cw, ch;
    int restored; /* placement came from a saved _Havel_CANVAS_GEOM, not from a fresh guess */
    Window transient_for;   /* WM_TRANSIENT_FOR: the window this dialog belongs to, else None */
    int isfullscreen;
    double saved_cx, saved_cy;
    int saved_cw, saved_ch;
    int workspace;
} Client;
typedef struct {
    int sw, sh;
    double cx, cy, zoom;
} Viewport;
struct Havel {
    Display  *dpy;
    Window    root, overlay, wm_check;
    Pixmap    backbuf[2];
    Picture   backbuf_pic[2];
    int       buf_busy[2];   /* server may still be presenting/reading this buffer */
    int       cur_buf;       /* buffer to draw into next */
    int       dirty;         /* a redraw is pending */
    uint32_t  present_serial;
    int       sw, sh;
    Viewport  vp;
    Client    clients[MAX_CLIENTS];
    int       nclients;
    Window    overrides[MAX_CLIENTS];
    Damage    override_damage[MAX_CLIENTS];
    int       noverride;
    Window    focused;
    int       damage_event;
    int       xi_opcode;
    int       present_opcode;
    Window    hover_win;   /* client currently parked under the real cursor */
    int    panning, pan_sx, pan_sy;
    double pan_vx, pan_vy;
    int    moving, move_sx, move_sy;
    double move_cx, move_cy;
    Window move_win;
    int    resizing, resize_sx, resize_sy, resize_cw0, resize_ch0;
    double resize_cx0, resize_cy0;
    int    resize_left, resize_top;   /* which edges this drag moves (else right/bottom), by start quadrant */
    Window resize_win;
    double last_input;   /* CLOCK_MONOTONIC seconds, updated on any input event */
    int    idle_fired;   /* IDLE_LOCK_CMD already run for the current idle period */
    int    mod_grab;     /* we hold the pointer because GESTURE_MOD is down (sync_gesture_grab()) */
    Atom   wm_protocols, wm_delete;   /* ICCCM window-close handshake, see close_client() */
    Atom   net_wm_state, net_wm_fullscreen;
    int    current_workspace;
    Viewport workspaces[MAX_WORKSPACES];
};

static int g_other_wm;
static Client *focus_raise(Havel *, Client *);
static void sync_gesture_grab(Havel *);
static void cleanup(Havel *);

/* Havel: one translation unit, split into ordered implementation fragments.
 * Edit config.h and reload with `make reload`. */

#include "modules/state.h"
#include "modules/canvas.h"
#include "modules/clients.h"
#include "modules/interaction.h"
#include "modules/events.h"
#include "modules/input.h"
#include "modules/mainloop.h"
