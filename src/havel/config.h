/* Havel configuration. This is the only config file; edit and (make reload). */
#ifndef HAVEL_CONFIG_H
#define HAVEL_CONFIG_H

#define MODKEY                 Mod4Mask
#define GESTURE_MOD            MODKEY
#define MAX_CLIENTS            64
#define MAX_WORKSPACES         9

/* Canvas movement, zoom, snapping and size behavior. */
#define ZOOM_MIN               0.05
#define ZOOM_MAX               1.0
#define ZOOM_SPEED             1.1
#define SNAP_ENABLED           1
#define SNAP_DIST              12
#define SNAP_GAP               10
#define SIZE_DETENT_ENABLED    1
#define SIZE_DETENT_DIST       16
#define SIZE_DETENT_LEVELS     5
#define VIEW_MARGIN            1.06
#define IDLE_LOCK_TIMEOUT      300
#define IDLE_LOCK_CMD          "dm-tool lock"

/* Mouse buttons for zooming (usually 4 for scroll up, 5 for scroll down) */
#define ZOOM_IN_BUTTON         4
#define ZOOM_OUT_BUTTON        5

/* Colors and Appearance. */
static const char *background = "#73a1d2";
static const char *bordercolor = "#28507B";
static const char *focuscolor = "#73a1d2";
static const unsigned int borderpx = 5;

static const unsigned int focusfollowsmouse = 1;

/* Just Like dwm. */
static const char *termcmd[]  = { "st", NULL };
static const char *dmenucmd[] = { "dmenu_run", NULL };
static const char *quitcmd[]  = { "pkill", "-x", "havel", NULL };

/* Canvas navigation directions */
enum { DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN };

/* 
 * Keybindings:
 * modifier, keysym, function, argument 
 */
static const Key keys[] = {
    /* Spawning and window management */
    { MODKEY,           XK_Return, spawn,                 { .v = termcmd } },
    { MODKEY,           XK_d,      spawn,                 { .v = dmenucmd } },
    { MODKEY,           XK_q,      kill_window,           { .i = 0 } },
    { MODKEY|ShiftMask, XK_r,      reload_havel,          { .i = 0 } },
    { MODKEY|ShiftMask, XK_e,      spawn,                 { .v = quitcmd } },

    /* View and zoom */
    { MODKEY|ShiftMask, XK_d,      lookat_focused_window, { .i = 0 } },
    { MODKEY,           XK_z,      zoom_in,               { .i = 0 } },
    { MODKEY|ShiftMask, XK_z,      zoom_out,              { .i = 0 } },

    /* Fullscreen and Vim-like canvas navigation */
    { MODKEY,           XK_f,      fullscreen_toggle,     { .i = 0 } },
    { MODKEY,           XK_h,      jump_to_window,        { .i = DIR_LEFT } },
    { MODKEY,           XK_l,      jump_to_window,        { .i = DIR_RIGHT } },
    { MODKEY,           XK_j,      jump_to_window,        { .i = DIR_DOWN } },
    { MODKEY,           XK_k,      jump_to_window,        { .i = DIR_UP } },

    /* Workspaces (Switch) */
    { MODKEY,           XK_1,      switch_workspace,      { .i = 1 } },
    { MODKEY,           XK_2,      switch_workspace,      { .i = 2 } },
    { MODKEY,           XK_3,      switch_workspace,      { .i = 3 } },
    { MODKEY,           XK_4,      switch_workspace,      { .i = 4 } },
    { MODKEY,           XK_5,      switch_workspace,      { .i = 5 } },
    { MODKEY,           XK_6,      switch_workspace,      { .i = 6 } },
    { MODKEY,           XK_7,      switch_workspace,      { .i = 7 } },
    { MODKEY,           XK_8,      switch_workspace,      { .i = 8 } },
    { MODKEY,           XK_9,      switch_workspace,      { .i = 9 } },

    /* Workspaces (Move window to) */
    { MODKEY|ShiftMask, XK_1,      move_to_workspace,     { .i = 1 } },
    { MODKEY|ShiftMask, XK_2,      move_to_workspace,     { .i = 2 } },
    { MODKEY|ShiftMask, XK_3,      move_to_workspace,     { .i = 3 } },
    { MODKEY|ShiftMask, XK_4,      move_to_workspace,     { .i = 4 } },
    { MODKEY|ShiftMask, XK_5,      move_to_workspace,     { .i = 5 } },
    { MODKEY|ShiftMask, XK_6,      move_to_workspace,     { .i = 6 } },
    { MODKEY|ShiftMask, XK_7,      move_to_workspace,     { .i = 7 } },
    { MODKEY|ShiftMask, XK_8,      move_to_workspace,     { .i = 8 } },
    { MODKEY|ShiftMask, XK_9,      move_to_workspace,     { .i = 9 } },
};

#endif /* HAVEL_CONFIG_H */
