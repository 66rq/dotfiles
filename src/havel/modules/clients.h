/* Havel implementation fragment: clients.h
 * Included in order by havel.c; not compiled separately. */

static Client *manage(Havel *z, Window w) {
    if (z->nclients >= MAX_CLIENTS) return NULL;
    if (find(z, w)) return NULL;
    XWindowAttributes a;
    if (!XGetWindowAttributes(z->dpy, w, &a)) return NULL;

    Client *c = &z->clients[z->nclients];
    memset(c, 0, sizeof *c);
    c->window = w;
    c->visual = a.visual;
    c->workspace = z->current_workspace;
    c->restored = load_client_geom(z, w, &c->cx, &c->cy);
    if (!c->restored) to_canvas(&z->vp, a.x, a.y, &c->cx, &c->cy);
    c->cw = a.width; c->ch = a.height;
    c->transient_for = None;
    if (!XGetTransientForHint(z->dpy, w, &c->transient_for)) c->transient_for = None;

    XSetWindowBorderWidth(z->dpy, w, 0);
    park(z, c);
    XGrabButton(z->dpy, Button1, AnyModifier, w, False, ButtonPressMask,
                GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(z->dpy, Button3, AnyModifier, w, False, ButtonPressMask,
                GrabModeSync, GrabModeAsync, None, None);
    c->damage = XDamageCreate(z->dpy, w, XDamageReportNonEmpty);
    z->nclients++;
    return c;
}

static void manage_override(Havel *z, Window w) {
    for (int i = 0; i < z->noverride; i++) if (z->overrides[i] == w) return;
    if (z->noverride >= MAX_CLIENTS) return;

    XWindowAttributes a;
    if (XGetWindowAttributes(z->dpy, w, &a))
        XSetWindowBorderWidth(z->dpy, w, 0);

    int i = z->noverride++;
    z->overrides[i] = w;
    z->override_damage[i] = XDamageCreate(z->dpy, w, XDamageReportNonEmpty);
}

static void unmanage_override(Havel *z, Window w) {
    for (int i = 0; i < z->noverride; i++) {
        if (z->overrides[i] != w) continue;
        if (z->override_damage[i]) XDamageDestroy(z->dpy, z->override_damage[i]);
        int last = --z->noverride;
        z->overrides[i] = z->overrides[last];
        z->override_damage[i] = z->override_damage[last];
        z->overrides[last] = None;
        z->override_damage[last] = None;
        return;
    }
}
