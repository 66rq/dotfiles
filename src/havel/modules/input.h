
static void spawn(Havel *z, const Arg *arg) {
    const char *const *cmd = arg ? (const char *const *)arg->v : NULL;
    if (!cmd || !cmd[0]) return;
    pid_t child = fork();
    if (child < 0) { perror("havel: fork"); return; }
    if (child == 0) {
        pid_t grandchild = fork();
        if (grandchild < 0) _exit(127);
        if (grandchild > 0) _exit(0);
        if (z && z->dpy) close(ConnectionNumber(z->dpy));
        (void)setsid();
        execvp(cmd[0], (char *const *)cmd);
        perror("havel: execvp");
        _exit(127);
    }
    while (waitpid(child, NULL, 0) < 0 && errno == EINTR) {}
}
static void kill_window(Havel *z, const Arg *arg) {
    (void)arg;
    if (z->focused) close_client(z, z->focused);
}
static void reload_havel(Havel *z, const Arg *arg) {
    (void)arg;
    restart(z);
}
static void lookat_focused_window(Havel *z, const Arg *arg) {
    (void)arg;
    view_focused(z);
    save_viewport(z);
}

static void on_key(Havel *z, XKeyEvent *ev) {
    for (int i = 0; i < NBINDINGS; i++) {
        const Key *k = &keys[i];
        if (ev->keycode != XKeysymToKeycode(z->dpy, k->keysym)
                || (ev->state & ~(numlockmask | LockMask)) != k->mod)
            continue;
        if (k->func) k->func(z, &k->arg);
        return;
    }
}

static void fullscreen_toggle(Havel *z, const Arg *arg) {
    (void)arg;
    Client *c = find(z, z->focused);
    if (!c) return;

    if (!c->isfullscreen) {
        c->saved_cx = c->cx;
        c->saved_cy = c->cy;
        c->saved_cw = c->cw;
        c->saved_ch = c->ch;
        c->isfullscreen = 1;
        
        int mx, my, mw, mh;
        pointer_monitor(z, &mx, &my, &mw, &mh);
        
        double cx1, cy1, cx2, cy2;
        to_canvas(&z->vp, mx, my, &cx1, &cy1);
        to_canvas(&z->vp, mx + mw, my + mh, &cx2, &cy2);
        
        c->cx = cx1;
        c->cy = cy1;
        c->cw = (int)(cx2 - cx1);
        c->ch = (int)(cy2 - cy1);
    } else {
        c->isfullscreen = 0;
        c->cx = c->saved_cx;
        c->cy = c->saved_cy;
        c->cw = c->saved_cw;
        c->ch = c->saved_ch;
    }
    
    park(z, c);
    save_client_geom(z, c);
    request_redraw(z);
}

static void jump_to_window(Havel *z, const Arg *arg) {
    Client *c = find(z, z->focused);
    if (!c) return;

    Client *target = NULL;
    double best_dist = 999999999.0;
    double cx = c->cx + c->cw / 2.0;
    double cy = c->cy + c->ch / 2.0;

    for (int i = 0; i < z->nclients; i++) {
        Client *it = &z->clients[i];
        if (it == c) continue;
        if (it->workspace != z->current_workspace) continue;

        double ix = it->cx + it->cw / 2.0;
        double iy = it->cy + it->ch / 2.0;
        double dx = ix - cx;
        double dy = iy - cy;
        int match = 0;

        switch (arg->i) {
            case DIR_LEFT:  match = (dx < 0); break;
            case DIR_RIGHT: match = (dx > 0); break;
            case DIR_UP:    match = (dy < 0); break;
            case DIR_DOWN:  match = (dy > 0); break;
        }

        if (match) {
            double dist = fabs(dx) + fabs(dy);
            if (dist < best_dist) {
                best_dist = dist;
                target = it;
            }
        }
    }

    if (!target) return;

    int mx, my, mw, mh;
    pointer_monitor(z, &mx, &my, &mw, &mh);
    
    double target_cx = target->cx + target->cw / 2.0;
    double target_cy = target->cy + target->ch / 2.0;
    double screen_cx = mx + mw / 2.0;
    double screen_cy = my + mh / 2.0;

    z->vp.cx = target_cx - (screen_cx - z->vp.sw / 2.0) / z->vp.zoom;
    z->vp.cy = target_cy - (screen_cy - z->vp.sh / 2.0) / z->vp.zoom;

    z->focused = target->window;
    XSetInputFocus(z->dpy, target->window, RevertToPointerRoot, CurrentTime);
    
    save_viewport(z);
    request_redraw(z);
}

static void zoom_in(Havel *z, const Arg *arg) {
    (void)arg;
    int rx = z->sw / 2, ry = z->sh / 2;
    Window r, ch; int wx, wy; unsigned int mask;
    XQueryPointer(z->dpy, z->root, &r, &ch, &rx, &ry, &wx, &wy, &mask);
    zoom_at(&z->vp, rx, ry, ZOOM_SPEED);
    save_viewport(z);
    request_redraw(z);
}

static void zoom_out(Havel *z, const Arg *arg) {
    (void)arg;
    int rx = z->sw / 2, ry = z->sh / 2;
    Window r, ch; int wx, wy; unsigned int mask;
    XQueryPointer(z->dpy, z->root, &r, &ch, &rx, &ry, &wx, &wy, &mask);
    zoom_at(&z->vp, rx, ry, 1.0 / ZOOM_SPEED);
    save_viewport(z);
    request_redraw(z);
}

static void switch_workspace(Havel *z, const Arg *arg) {
    int target = arg->i - 1;
    if (target < 0 || target >= MAX_WORKSPACES || target == z->current_workspace) return;

    if (z->hover_win) {
        Client *h = find(z, z->hover_win);
        if (h) park(z, h);
        z->hover_win = 0;
    }

    z->workspaces[z->current_workspace] = z->vp;
    z->current_workspace = target;
    z->vp = z->workspaces[z->current_workspace];
    
    z->focused = 0;
    for (int i = z->nclients - 1; i >= 0; i--) {
        if (z->clients[i].workspace == target) {
            z->focused = z->clients[i].window;
            break;
        }
    }
    XSetInputFocus(z->dpy, z->focused ? z->focused : z->root,
                   RevertToPointerRoot, CurrentTime);

    save_viewport(z);
    request_redraw(z);
    
    long data = target;
    Atom net_cur = XInternAtom(z->dpy, "_NET_CURRENT_DESKTOP", False);
    XChangeProperty(z->dpy, z->root, net_cur, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&data, 1);
}

static void move_to_workspace(Havel *z, const Arg *arg) {
    int target = arg->i - 1;
    if (target < 0 || target >= MAX_WORKSPACES) return;
    Client *c = find(z, z->focused);
    if (!c || c->workspace == target) return;

    if (z->hover_win == c->window) {
        park(z, c);
        z->hover_win = 0;
    }
    c->workspace = target;
    
    long data = target;
    Atom net_wm_desktop = XInternAtom(z->dpy, "_NET_WM_DESKTOP", False);
    XChangeProperty(z->dpy, c->window, net_wm_desktop, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&data, 1);
    
    z->focused = 0;
    for (int i = z->nclients - 1; i >= 0; i--) {
        if (z->clients[i].workspace == z->current_workspace) {
            z->focused = z->clients[i].window;
            break;
        }
    }
    XSetInputFocus(z->dpy, z->focused ? z->focused : z->root,
                   RevertToPointerRoot, CurrentTime);

    request_redraw(z);
}
