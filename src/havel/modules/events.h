/* Havel implementation fragment: events.h
 * Included in order by havel.c; not compiled separately. */

static void on_map(Havel *z, XMapRequestEvent *ev) {
    XWindowAttributes a;
    if (!XGetWindowAttributes(z->dpy, ev->window, &a)) return;
    if (a.override_redirect) { XMapWindow(z->dpy, ev->window); return; }
    Client *c = find(z, ev->window);
    if (!c && (c = manage(z, ev->window))) {
        int mx, my, mw, mh;
        double center_cx, center_cy;

        pointer_monitor(z, &mx, &my, &mw, &mh);

        to_canvas(&z->vp,
                  mx + mw / 2.0, my + mh / 2.0,
                  &center_cx, &center_cy);

        c->cx = center_cx - c->cw / 2.0;
        c->cy = center_cy - c->ch / 2.0;

        save_client_geom(z, c);
    }
    XMapWindow(z->dpy, ev->window);
    if (c) { z->focused = c->window; XSetInputFocus(z->dpy, c->window, RevertToPointerRoot, CurrentTime); }
    request_redraw(z);
}

static void on_unmap(Havel *z, XUnmapEvent *ev) {
    if (!ev->send_event) {
        unmanage(z, ev->window); unmanage_override(z, ev->window); request_redraw(z);
    }
}

static void on_destroy(Havel *z, XDestroyWindowEvent *ev) {
    unmanage(z, ev->window); unmanage_override(z, ev->window); request_redraw(z);
}

static void on_mapnotify(Havel *z, XMapEvent *ev) {
    if (ev->window == z->overlay) return;
    XWindowAttributes a;
    if (!XGetWindowAttributes(z->dpy, ev->window, &a)) return;
    if (a.override_redirect) { manage_override(z, ev->window); request_redraw(z); }
}

static void on_configure(Havel *z, XConfigureRequestEvent *ev) {
    Client *c = find(z, ev->window);
    if (c) {
        if (ev->value_mask & CWWidth)  c->cw = ev->width;
        if (ev->value_mask & CWHeight) c->ch = ev->height;
        park(z, c);
    } else {
        XWindowChanges wc = { .x = ev->x, .y = ev->y,
            .width = ev->width, .height = ev->height,
            .border_width = ev->border_width, .sibling = ev->above, .stack_mode = ev->detail };
        XConfigureWindow(z->dpy, ev->window, ev->value_mask, &wc);
    }
}

static Client *focus_raise(Havel *z, Client *c) {
    raise_client(z, c);
    c = &z->clients[z->nclients - 1];
    z->focused = c->window;
    XRaiseWindow(z->dpy, c->window);
    XSetInputFocus(z->dpy, c->window, RevertToPointerRoot, CurrentTime);
    request_redraw(z);
    return c;
}
