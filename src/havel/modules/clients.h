/* Havel implementation fragment: clients.h
 * Included in order by havel.c; not compiled separately. */

static Client *manage(Havel *z, Window w) {
    if (z->nclients == MAX_CLIENTS) return NULL;
    XWindowAttributes a;
    if (!XGetWindowAttributes(z->dpy, w, &a)) return NULL;
    Client *c = &z->clients[z->nclients++];
    c->window = w;
    c->workspace = z->current_workspace;
    c->restored = load_client_geom(z, w, &c->cx, &c->cy);
    if (!c->restored) to_canvas(&z->vp, a.x, a.y, &c->cx, &c->cy);
    c->cw = a.width; c->ch = a.height; c->depth = a.depth;
    if (!XGetTransientForHint(z->dpy, w, &c->transient_for)) c->transient_for = None;
    XSetWindowBorderWidth(z->dpy, w, 0);
    park(z, c);
    XGrabButton(z->dpy, Button1, AnyModifier, w, False, ButtonPressMask,
                GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(z->dpy, Button3, AnyModifier, w, False, ButtonPressMask,
                GrabModeSync, GrabModeAsync, None, None);
    XDamageCreate(z->dpy, w, XDamageReportNonEmpty);
    return c;
}

static void manage_override(Havel *z, Window w) {
    for (int i = 0; i < z->noverride; i++) if (z->overrides[i] == w) return;
    if (z->noverride == MAX_CLIENTS) return;
    z->overrides[z->noverride++] = w;
    XWindowAttributes a;
    if (XGetWindowAttributes(z->dpy, w, &a))
        XSetWindowBorderWidth(z->dpy, w, 0);
    XDamageCreate(z->dpy, w, XDamageReportNonEmpty);
}

static void unmanage_override(Havel *z, Window w) {
    for (int i = 0; i < z->noverride; i++)
        if (z->overrides[i] == w) { z->overrides[i] = z->overrides[--z->noverride]; return; }
}
