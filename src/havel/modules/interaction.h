/* Havel implementation fragment: interaction.h
 * Included in order by havel.c; not compiled separately. */

static void sync_gesture_grab(Havel *z) {
    Window r, ch; int rx, ry, wx, wy; unsigned int mask;
    if (!XQueryPointer(z->dpy, z->root, &r, &ch, &rx, &ry, &wx, &wy, &mask)) return;
    if (!!(mask & GESTURE_MOD) == z->mod_grab) return;
    if (mask & GESTURE_MOD)
        z->mod_grab = XGrabPointer(z->dpy, z->overlay, False,
                                   ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
                                   GrabModeAsync, GrabModeAsync, None, None, CurrentTime) == GrabSuccess;
    else {
        z->mod_grab = 0;
        if (!z->moving && !z->resizing && !z->panning) XUngrabPointer(z->dpy, CurrentTime);
    }
}

static void close_client(Havel *z, Window w) {
    Atom *protos = NULL;
    int n = 0, supported = 0;
    if (XGetWMProtocols(z->dpy, w, &protos, &n)) {
        for (int i = 0; i < n; i++) if (protos[i] == z->wm_delete) supported = 1;
        if (protos) XFree(protos);
    }
    if (!supported) { XKillClient(z->dpy, w); return; }
    XEvent ev = { .xclient = { .type = ClientMessage, .window = w,
                                .message_type = z->wm_protocols, .format = 32 } };
    ev.xclient.data.l[0] = (long)z->wm_delete;
    ev.xclient.data.l[1] = CurrentTime;
    XSendEvent(z->dpy, w, False, NoEventMask, &ev);
}

static void unmanage(Havel *z, Window w) {
    int i;
    for (i = 0; i < z->nclients; i++) if (z->clients[i].window == w) break;
    if (i == z->nclients) return;

    Client *c = &z->clients[i];
    Window parent = c->transient_for;

    XUngrabButton(z->dpy, Button1, AnyModifier, w);
    XUngrabButton(z->dpy, Button3, AnyModifier, w);
    if (c->damage) XDamageDestroy(z->dpy, c->damage);

    int last = --z->nclients;
    z->clients[i] = z->clients[last];
    memset(&z->clients[last], 0, sizeof z->clients[last]);

    if (z->move_win == w || z->resize_win == w) {
        z->moving = z->resizing = 0; z->move_win = z->resize_win = 0;
        XUngrabPointer(z->dpy, CurrentTime);
        z->mod_grab = 0; sync_gesture_grab(z);
    }
    if (z->hover_win == w) z->hover_win = 0;

    if (z->focused == w) {
        Client *back = parent ? find(z, parent) : NULL;
        z->focused = 0;
        if (back && back->workspace == z->current_workspace) {
            z->focused = back->window;
        } else {
            for (int j = z->nclients - 1; j >= 0; j--) {
                if (z->clients[j].workspace == z->current_workspace) {
                    z->focused = z->clients[j].window;
                    break;
                }
            }
        }
        XSetInputFocus(z->dpy, z->focused ? z->focused : z->root,
                       RevertToPointerRoot, CurrentTime);
    }
}
