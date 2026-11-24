/* Havel implementation fragment: state.h
 * Included in order by havel.c; not compiled separately. */

static int xerr(Display *d, XErrorEvent *e) { (void)d; if (e->error_code == BadAccess) g_other_wm = 1; return 0; }

static double now_secs(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

#define STATE_PROP    "_ZZWM_STATE"
#define STATE_VERSION 2
#define STATE_HDR     7
#define STATE_STRIDE  6

typedef struct {
    int    valid;
    double cx, cy, zoom;
    Window focused;
    int    n;
    int    current_workspace;
    Viewport workspaces[MAX_WORKSPACES];
    struct { Window w; double cx, cy; int workspace; } c[MAX_CLIENTS];
} SavedState;

static SavedState g_saved;
static char **g_argv;
static volatile sig_atomic_t g_restart_req;

static void on_sigusr1(int sig) { (void)sig; g_restart_req = 1; }

static void save_state(Havel *z) {
    long buf[STATE_HDR + MAX_CLIENTS * STATE_STRIDE];
    int n = 0;
    buf[n++] = STATE_VERSION;
    buf[n++] = lround(z->vp.cx * 256.0);
    buf[n++] = lround(z->vp.cy * 256.0);
    buf[n++] = lround(z->vp.zoom * 65536.0);
    buf[n++] = (long)z->focused;
    buf[n++] = z->nclients;
    buf[n++] = z->current_workspace;
    for (int i = 0; i < MAX_WORKSPACES; i++) {
        buf[n++] = lround(z->workspaces[i].cx * 256.0);
        buf[n++] = lround(z->workspaces[i].cy * 256.0);
        buf[n++] = lround(z->workspaces[i].zoom * 65536.0);
    }
    for (int i = 0; i < z->nclients; i++) {
        Client *c = &z->clients[i];
        buf[n++] = (long)c->window;
        buf[n++] = lround(c->cx * 256.0);
        buf[n++] = lround(c->cy * 256.0);
        buf[n++] = c->cw;
        buf[n++] = c->ch;
        buf[n++] = c->workspace;
    }
    XChangeProperty(z->dpy, z->root, XInternAtom(z->dpy, STATE_PROP, False),
                    XA_CARDINAL, 32, PropModeReplace, (unsigned char *)buf, n);
}

static void load_state(Display *dpy, Window root, SavedState *s) {
    Atom prop = XInternAtom(dpy, STATE_PROP, True), type;
    int fmt; unsigned long nitems, after; unsigned char *data = NULL;
    s->valid = 0;
    if (prop == None) return;
    if (XGetWindowProperty(dpy, root, prop, 0, STATE_HDR + MAX_CLIENTS * STATE_STRIDE,
                           False, XA_CARDINAL, &type, &fmt, &nitems, &after, &data)
            != Success)
        return;
    long *v = (long *)data;
    if (data && fmt == 32 && nitems >= STATE_HDR && (int32_t)(uint32_t)v[0] == STATE_VERSION) {
        s->cx      = (int32_t)(uint32_t)v[1] / 256.0;
        s->cy      = (int32_t)(uint32_t)v[2] / 256.0;
        s->zoom    = (int32_t)(uint32_t)v[3] / 65536.0;
        s->focused = (Window)(uint32_t)v[4];
        s->n       = (int32_t)(uint32_t)v[5];
        s->current_workspace = (int32_t)(uint32_t)v[6];
        if (s->current_workspace < 0 || s->current_workspace >= MAX_WORKSPACES) s->current_workspace = 0;
        int offset = 7;
        for (int i = 0; i < MAX_WORKSPACES; i++) {
            s->workspaces[i].cx = (int32_t)(uint32_t)v[offset + i*3] / 256.0;
            s->workspaces[i].cy = (int32_t)(uint32_t)v[offset + i*3 + 1] / 256.0;
            s->workspaces[i].zoom = (int32_t)(uint32_t)v[offset + i*3 + 2] / 65536.0;
        }
        offset += MAX_WORKSPACES * 3;
        if (s->n < 0) s->n = 0;
        if (s->n > MAX_CLIENTS) s->n = MAX_CLIENTS;
        if ((unsigned long)(offset + s->n * STATE_STRIDE) > nitems) s->n = 0;
        if (s->zoom < ZOOM_MIN || s->zoom > ZOOM_MAX) s->zoom = 1.0;
        for (int i = 0; i < s->n; i++) {
            long *e = v + offset + i * STATE_STRIDE;
            s->c[i].w  = (Window)(uint32_t)e[0];
            s->c[i].cx = (int32_t)(uint32_t)e[1] / 256.0;
            s->c[i].cy = (int32_t)(uint32_t)e[2] / 256.0;
            s->c[i].workspace = (int32_t)(uint32_t)e[5];
        }
        s->valid = 1;
    }
    if (data) XFree(data);
}

#define GEOM_PROP "_ZZWM_CANVAS_GEOM"
#define VIEW_PROP "_ZZWM_VIEWPORT"

static void save_client_geom(Havel *z, Client *c) {
    long g[4] = { lround(c->cx * 256.0), lround(c->cy * 256.0), c->cw, c->ch };
    XChangeProperty(z->dpy, c->window, XInternAtom(z->dpy, GEOM_PROP, False),
                    XA_CARDINAL, 32, PropModeReplace, (unsigned char *)g, 4);
}

static int load_client_geom(Havel *z, Window w, double *cx, double *cy) {
    Atom prop = XInternAtom(z->dpy, GEOM_PROP, True), type;
    int fmt; unsigned long nitems, after; unsigned char *data = NULL;
    if (prop == None) return 0;
    if (XGetWindowProperty(z->dpy, w, prop, 0, 4, False, XA_CARDINAL,
                           &type, &fmt, &nitems, &after, &data) != Success)
        return 0;
    int ok = 0;
    if (data && fmt == 32 && nitems >= 2) {
        long *v = (long *)data;
        *cx = (int32_t)(uint32_t)v[0] / 256.0;
        *cy = (int32_t)(uint32_t)v[1] / 256.0;
        ok = 1;
    }
    if (data) XFree(data);
    return ok;
}

static void save_viewport(Havel *z) {
    z->workspaces[z->current_workspace] = z->vp;
    long v[3] = { lround(z->vp.cx * 256.0), lround(z->vp.cy * 256.0),
                  lround(z->vp.zoom * 65536.0) };
    XChangeProperty(z->dpy, z->root, XInternAtom(z->dpy, VIEW_PROP, False),
                    XA_CARDINAL, 32, PropModeReplace, (unsigned char *)v, 3);
}

static int load_viewport(Display *dpy, Window root, Viewport *vp) {
    Atom prop = XInternAtom(dpy, VIEW_PROP, True), type;
    int fmt; unsigned long nitems, after; unsigned char *data = NULL;
    if (prop == None) return 0;
    if (XGetWindowProperty(dpy, root, prop, 0, 3, False, XA_CARDINAL,
                           &type, &fmt, &nitems, &after, &data) != Success)
        return 0;
    int ok = 0;
    if (data && fmt == 32 && nitems >= 3) {
        long *v = (long *)data;
        double zoom = (int32_t)(uint32_t)v[2] / 65536.0;
        if (zoom >= ZOOM_MIN && zoom <= ZOOM_MAX) {
            vp->cx = (int32_t)(uint32_t)v[0] / 256.0;
            vp->cy = (int32_t)(uint32_t)v[1] / 256.0;
            vp->zoom = zoom;
            ok = 1;
        }
    }
    if (data) XFree(data);
    return ok;
}

static void restart(Havel *z) {
    save_state(z);
    XUngrabPointer(z->dpy, CurrentTime);
    XUngrabKeyboard(z->dpy, CurrentTime);
    XSync(z->dpy, False);
    XCloseDisplay(z->dpy);
    execvp(g_argv[0], g_argv);
    char *fallback[] = { (char *)"havel", NULL };
    execvp(fallback[0], fallback);
    fputs("havel: restart failed to exec\n", stderr);
    _exit(1);
}

static void mark_activity(Havel *z) {
    z->last_input = now_secs();
    z->idle_fired = 0;
}
