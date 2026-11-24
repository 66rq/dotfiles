/* Canvas engine: coordinate transforms, zoom/fit, hit testing, rendering, and gestures. */

/* Havel implementation fragment: geometry.h
 * Included in order by havel.c; not compiled separately. */

static void to_screen(Viewport *v, double cx, double cy, double *sx, double *sy) {
    *sx = (cx - v->cx) * v->zoom + v->sw / 2.0;
    *sy = (cy - v->cy) * v->zoom + v->sh / 2.0;
}

static void to_canvas(Viewport *v, double sx, double sy, double *cx, double *cy) {
    *cx = (sx - v->sw / 2.0) / v->zoom + v->cx;
    *cy = (sy - v->sh / 2.0) / v->zoom + v->cy;
}

static void zoom_at(Viewport *v, double sx, double sy, double f) {
    double cx, cy, zoom = v->zoom * f;
    to_canvas(v, sx, sy, &cx, &cy);
    v->zoom = zoom < ZOOM_MIN ? ZOOM_MIN : zoom > ZOOM_MAX ? ZOOM_MAX : zoom;
    v->cx = cx - (sx - v->sw / 2.0) / v->zoom;
    v->cy = cy - (sy - v->sh / 2.0) / v->zoom;
}

static void srect(Client *c, Viewport *v, int *x, int *y, int *w, int *h) {
    double sx, sy;
    to_screen(v, c->cx, c->cy, &sx, &sy);
    *x = (int)sx; *y = (int)sy;
    *w = (int)(c->cw * v->zoom); if (*w < 1) *w = 1;
    *h = (int)(c->ch * v->zoom); if (*h < 1) *h = 1;
}

static Client *find(Havel *z, Window w) {
    for (int i = 0; i < z->nclients; i++)
        if (z->clients[i].window == w) return &z->clients[i];
    return NULL;
}

static Client *hit(Havel *z, int sx, int sy) {
    for (int i = z->nclients - 1; i >= 0; i--) {
        if (z->clients[i].workspace != z->current_workspace) continue;
        int x, y, w, h;
        srect(&z->clients[i], &z->vp, &x, &y, &w, &h);
        if (sx >= x && sx < x+w && sy >= y && sy < y+h) return &z->clients[i];
    }
    return NULL;
}

static Window hit_override(Havel *z, int sx, int sy, XWindowAttributes *out) {
    for (int i = z->noverride - 1; i >= 0; i--) {
        Window w = z->overrides[i];
        XWindowAttributes a;
        if (!XGetWindowAttributes(z->dpy, w, &a) || a.map_state != IsViewable) continue;
        if (sx >= a.x && sx < a.x + a.width && sy >= a.y && sy < a.y + a.height) {
            *out = a;
            return w;
        }
    }
    return None;
}

static void to_client_local(Client *c, Viewport *v, int sx, int sy, int *wx, int *wy) {
    double cx, cy;
    to_canvas(v, sx, sy, &cx, &cy);
    *wx = (int)(cx - c->cx);
    *wy = (int)(cy - c->cy);
}

static void raise_client(Havel *z, Client *c) {
    int idx = (int)(c - z->clients);
    if (idx < 0 || idx >= z->nclients - 1) return;
    Client tmp = *c;
    for (int i = idx; i < z->nclients - 1; i++) z->clients[i] = z->clients[i + 1];
    z->clients[z->nclients - 1] = tmp;
}

static unsigned int numlockmask = 0;

static void update_numlockmask(Display *dpy) {
    numlockmask = 0;
    KeyCode nl = XKeysymToKeycode(dpy, XK_Num_Lock);
    if (!nl) return;
    XModifierKeymap *m = XGetModifierMapping(dpy);
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < m->max_keypermod; j++)
            if (m->modifiermap[i * m->max_keypermod + j] == nl) numlockmask = 1 << i;
    XFreeModifiermap(m);
}

static void grab_key(Display *dpy, KeyCode code, unsigned int mod, Window w) {
    unsigned int locks[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
    for (int i = 0; i < 4; i++)
        XGrabKey(dpy, code, mod|locks[i], w, True, GrabModeAsync, GrabModeAsync);
}

static void park(Havel *z, Client *c) {
    XMoveResizeWindow(z->dpy, c->window, -(c->cw+32), -(c->ch+32), c->cw, c->ch);
}

static void pointer_monitor(Havel *z, int *mx, int *my, int *mw, int *mh) {
    *mx = 0;
    *my = 0;
    *mw = z->sw;
    *mh = z->sh;

    Window root_return, child_return;
    int rx, ry, wx, wy;
    unsigned int mask;

    if (!XQueryPointer(z->dpy, z->root, &root_return, &child_return,
                       &rx, &ry, &wx, &wy, &mask))
        return;

    int rr_event, rr_error;
    int rr_major = 1, rr_minor = 5;

    if (!XRRQueryExtension(z->dpy, &rr_event, &rr_error) ||
        !XRRQueryVersion(z->dpy, &rr_major, &rr_minor) ||
        rr_major < 1 || (rr_major == 1 && rr_minor < 5))
        return;

    int nmon = 0;
    XRRMonitorInfo *monitors = XRRGetMonitors(
        z->dpy, z->root, True, &nmon
    );

    if (!monitors || nmon <= 0) {
        if (monitors) XRRFreeMonitors(monitors);
        return;
    }

    int selected = -1;
    long long best_distance = -1;

    for (int i = 0; i < nmon; i++) {
        XRRMonitorInfo *m = &monitors[i];

        if (rx >= m->x && rx < m->x + m->width &&
            ry >= m->y && ry < m->y + m->height) {
            selected = i;
            break;
        }

        long long dx = 0, dy = 0;

        if (rx < m->x)
            dx = (long long)m->x - rx;
        else if (rx >= m->x + m->width)
            dx = (long long)rx - (m->x + m->width - 1);

        if (ry < m->y)
            dy = (long long)m->y - ry;
        else if (ry >= m->y + m->height)
            dy = (long long)ry - (m->y + m->height - 1);

        long long distance = dx * dx + dy * dy;

        if (selected < 0 || best_distance < 0 ||
            distance < best_distance) {
            selected = i;
            best_distance = distance;
        }
    }

    if (selected >= 0) {
        *mx = monitors[selected].x;
        *my = monitors[selected].y;
        *mw = monitors[selected].width;
        *mh = monitors[selected].height;
    }

    XRRFreeMonitors(monitors);
}

static void view_focused(Havel *z) {
    Client *c = find(z, z->focused);
    if (!c) return;

    int mx, my, mw, mh;
    pointer_monitor(z, &mx, &my, &mw, &mh);

    z->vp.zoom = 1.0;

    double target_cx = c->cx + c->cw / 2.0;
    double target_cy = c->cy + c->ch / 2.0;
    double monitor_cx = mx + mw / 2.0;
    double monitor_cy = my + mh / 2.0;

    z->vp.cx = target_cx - (monitor_cx - z->vp.sw / 2.0) / z->vp.zoom;
    z->vp.cy = target_cy - (monitor_cy - z->vp.sh / 2.0) / z->vp.zoom;
}

static void composite_window(Havel *z, Window w, int depth, double zoom, int op,
                              Picture dst, int sx, int sy, int sw, int sh) {
    Pixmap pix = XCompositeNameWindowPixmap(z->dpy, w);
    if (!pix) return;
    Picture src = XRenderCreatePicture(z->dpy, pix,
        depth == 32 ? z->fmt32 : z->fmt24, 0, NULL);
    if (zoom > 0 && zoom != 1.0) {
        XFixed inv = XDoubleToFixed(1.0 / zoom);
        XTransform xf = {{{ inv, 0, 0 }, { 0, inv, 0 }, { 0, 0, XDoubleToFixed(1.0) }}};
        const char *filt = zoom > 1.0 ? FilterNearest : FilterBilinear;
        XRenderSetPictureFilter(z->dpy, src, filt, NULL, 0);
        XRenderSetPictureTransform(z->dpy, src, &xf);
    }
    XRenderComposite(z->dpy, op, src, None, dst, 0, 0, 0, 0, sx, sy, sw, sh);
    XRenderFreePicture(z->dpy, src);
    XFreePixmap(z->dpy, pix);
}


static XRenderColor havel_render_color(const char *value) {
    const char *hex = value && value[0] == '#' ? value + 1 : value;
    char *end = NULL;
    unsigned long rgb = (hex && strlen(hex) == 6) ? strtoul(hex, &end, 16) : 0;
    if (!hex || !end || *end != '\0') rgb = 0;
    XRenderColor c = {
        (unsigned short)(((rgb >> 16) & 0xff) * 257),
        (unsigned short)(((rgb >> 8) & 0xff) * 257),
        (unsigned short)((rgb & 0xff) * 257),
        0xffff
    };
    return c;
}

static Pixmap havel_wallpaper_pixmap = None;
static Picture havel_wallpaper_picture = None;
static unsigned int havel_wallpaper_width = 0, havel_wallpaper_height = 0;

static Pixmap havel_root_wallpaper(Havel *z) {
    const char *names[] = { "_XROOTPMAP_ID", "ESETROOT_PMAP_ID" };
    for (int i = 0; i < 2; ++i) {
        Atom property = XInternAtom(z->dpy, names[i], False);
        Atom actual = None;
        int format = 0;
        unsigned long nitems = 0, after = 0;
        unsigned char *data = NULL;
        if (XGetWindowProperty(z->dpy, z->root, property, 0, 1, False,
                XA_PIXMAP, &actual, &format, &nitems, &after, &data) == Success
                && actual == XA_PIXMAP && format == 32 && nitems == 1 && data) {
            Pixmap p = *(Pixmap *)data;
            XFree(data);
            if (p != None) return p;
        } else if (data) {
            XFree(data);
        }
    }
    return None;
}

static void draw_canvas_background(Havel *z, Picture dst) {
    XRenderColor bg = havel_render_color(background);
    XRectangle rect = { 0, 0, (unsigned short)z->sw, (unsigned short)z->sh };
    XRenderFillRectangles(z->dpy, PictOpSrc, dst, &bg, &rect, 1);
Pixmap p = havel_root_wallpaper(z);
    if (p != havel_wallpaper_pixmap) {
        if (havel_wallpaper_picture != None)
            XRenderFreePicture(z->dpy, havel_wallpaper_picture);
        havel_wallpaper_picture = None;
        havel_wallpaper_pixmap = None;
        havel_wallpaper_width = havel_wallpaper_height = 0;
        if (p != None) {
            Window root_return;
            int x, y;
            unsigned int width, height, border, depth;
            if (XGetGeometry(z->dpy, p, &root_return, &x, &y,
                    &width, &height, &border, &depth)
                    && depth == (unsigned int)DefaultDepth(z->dpy, DefaultScreen(z->dpy))) {
                XRenderPictFormat *fmt = XRenderFindVisualFormat(
                    z->dpy, DefaultVisual(z->dpy, DefaultScreen(z->dpy)));
                if (fmt != NULL) {
                    Picture pic = XRenderCreatePicture(z->dpy, p, fmt, 0, NULL);
                    if (pic != None) {
                        havel_wallpaper_picture = pic;
                        havel_wallpaper_pixmap = p;
                        havel_wallpaper_width = width;
                        havel_wallpaper_height = height;
                    }
                }
            }
        }
    }
    if (havel_wallpaper_picture != None) {
        unsigned int width = havel_wallpaper_width < (unsigned int)z->sw
            ? havel_wallpaper_width : (unsigned int)z->sw;
        unsigned int height = havel_wallpaper_height < (unsigned int)z->sh
            ? havel_wallpaper_height : (unsigned int)z->sh;
        if (width && height)
            XRenderComposite(z->dpy, PictOpSrc, havel_wallpaper_picture, None, dst,
                             0, 0, 0, 0, 0, 0, width, height);
    }
}

static void composite_frame(Havel *z) {
    Picture dst = z->backbuf_pic[z->cur_buf];
    draw_canvas_background(z, dst);

    for (int i = 0; i < z->nclients; i++) {
        Client *c = &z->clients[i];
        if (c->workspace != z->current_workspace) continue;
        
        if (c->isfullscreen) {
            int sx, sy, sw, sh;
            srect(c, &z->vp, &sx, &sy, &sw, &sh);
            composite_window(z, c->window, c->depth, z->vp.zoom, PictOpSrc, dst, sx, sy, sw, sh);
            continue; 
        }

        XRenderColor border_col =
            havel_render_color(c->window == z->focused ? focuscolor : bordercolor);

        int sx, sy, sw, sh;
        srect(c, &z->vp, &sx, &sy, &sw, &sh);

        int bw = (int)(borderpx * z->vp.zoom + 0.5);
        if (borderpx > 0 && z->vp.zoom > 0.0 && bw < 1)
            bw = 1;
        if (bw < 0)
            bw = 0;

        if (sx >= z->sw || sy >= z->sh ||
            sx + sw <= 0 || sy + sh <= 0)
            continue;

        if (bw > 0) {
            int ox = sx - bw;
            int oy = sy - bw;
            int ow = sw + 2 * bw;
            int oh = sh + 2 * bw;
            XRectangle frame = {
                (short)ox, (short)oy,
                (unsigned short)ow, (unsigned short)oh
            };
            XRenderFillRectangles(z->dpy, PictOpSrc, dst,
                                  &border_col, &frame, 1);
        }
        composite_window(z, c->window, c->depth, z->vp.zoom,
                         PictOpSrc, dst, sx, sy, sw, sh);
    }

    for (int i = 0; i < z->noverride; i++) {
        Window w = z->overrides[i];
        XWindowAttributes a;
        if (!XGetWindowAttributes(z->dpy, w, &a) ||
            a.map_state != IsViewable)
            continue;
        if (a.x >= z->sw || a.y >= z->sh ||
            a.x + a.width <= 0 || a.y + a.height <= 0)
            continue;
        composite_window(z, w, a.depth, 0, PictOpOver, dst,
                         a.x, a.y, a.width, a.height);
    }
}

static void present(Havel *z) {
    int i = z->cur_buf;
    XPresentPixmap(z->dpy, z->overlay, z->backbuf[i], ++z->present_serial,
                   None, None, 0, 0, None, None, None,
                   PresentOptionNone, 0, 0, 0, NULL, 0);
    XFlush(z->dpy);
    z->buf_busy[i] = 1;
    z->cur_buf = 1 - i;
    z->dirty = 0;
}

static void pump_redraw(Havel *z) {
    if (!z->dirty || z->buf_busy[z->cur_buf]) return;
    composite_frame(z);
    present(z);
}

static void request_redraw(Havel *z) {
    z->dirty = 1;
    pump_redraw(z);
}

static void on_present_idle(Havel *z, XPresentIdleNotifyEvent *e) {
    for (int i = 0; i < 2; i++)
        if (z->backbuf[i] == e->pixmap) z->buf_busy[i] = 0;
    pump_redraw(z);
}

static void on_button(Havel *z, XButtonEvent *ev) {
    if (ev->state & GESTURE_MOD) {
        XAllowEvents(z->dpy, AsyncPointer, ev->time);
        if (ev->button == ZOOM_IN_BUTTON)      { zoom_at(&z->vp, ev->x_root, ev->y_root, ZOOM_SPEED);     save_viewport(z); request_redraw(z); }
        else if (ev->button == ZOOM_OUT_BUTTON) { zoom_at(&z->vp, ev->x_root, ev->y_root, 1.0/ZOOM_SPEED); save_viewport(z); request_redraw(z); }
        else if (ev->button == 2 || (ev->button == 1 && (ev->state & ShiftMask))) {
            z->panning = 1; z->pan_sx = ev->x_root; z->pan_sy = ev->y_root;
            z->pan_vx = z->vp.cx; z->pan_vy = z->vp.cy;
        } else if (ev->button == 1 || ev->button == 3) {
            Client *c = hit(z, ev->x_root, ev->y_root);
            if (!c) return;
            c = focus_raise(z, c);
            if (ev->button == 1) {
                z->moving = 1; z->move_win = c->window;
                z->move_sx = ev->x_root; z->move_sy = ev->y_root;
                z->move_cx = c->cx;      z->move_cy = c->cy;
            } else if (ev->button == 3) {
                z->resizing = 1; z->resize_win = c->window;
                z->resize_sx = ev->x_root; z->resize_sy = ev->y_root;
                z->resize_cw0 = c->cw;     z->resize_ch0 = c->ch;
                z->resize_cx0 = c->cx;     z->resize_cy0 = c->cy;
                int wx, wy;
                to_client_local(c, &z->vp, ev->x_root, ev->y_root, &wx, &wy);
                z->resize_left = wx < c->cw / 2;
                z->resize_top  = wy < c->ch / 2;
            }
        }
        return;
    }

    Client *c = find(z, ev->window);
    if (c) focus_raise(z, c);
    XAllowEvents(z->dpy, ReplayPointer, ev->time);
}

static void on_release(Havel *z, XButtonEvent *ev) {
    if ((ev->button == 2 && z->panning) || (ev->button == 1 && (z->moving || z->panning)) || (ev->button == 3 && z->resizing)) {
        if (z->panning) save_viewport(z);
        Client *c = find(z, z->moving ? z->move_win : z->resize_win);
        if (c) save_client_geom(z, c);
        z->panning = z->moving = z->resizing = 0;
        z->move_win = z->resize_win = 0;
        XUngrabPointer(z->dpy, CurrentTime);
        z->mod_grab = 0;
        sync_gesture_grab(z);
    }
}

static void snap_translate(Havel *z, Client *c, double basex, double basey, double *dx, double *dy) {
    if (!SNAP_ENABLED) return;
    double l = basex + *dx, r = l + c->cw, t = basey + *dy, b = t + c->ch;
    double bestx = SNAP_DIST, besty = SNAP_DIST;
    Client *ox = NULL, *oy = NULL;
    for (int i = 0; i < z->nclients; i++) {
        Client *o = &z->clients[i];
        if (o == c) continue;
        double ol = o->cx, orr = o->cx + o->cw, ot = o->cy, ob = o->cy + o->ch;
        if (t < ob && b > ot) {
            double xc[4] = { ol - l, (orr + SNAP_GAP) - l, orr - r, (ol - SNAP_GAP) - r };
            for (int k = 0; k < 4; k++) if (fabs(xc[k]) < fabs(bestx)) { bestx = xc[k]; ox = o; }
        }
        if (l < orr && r > ol) {
            double yc[4] = { ot - t, (ob + SNAP_GAP) - t, ob - b, (ot - SNAP_GAP) - b };
            for (int k = 0; k < 4; k++) if (fabs(yc[k]) < fabs(besty)) { besty = yc[k]; oy = o; }
        }
    }
    if (ox && !oy) {
        double ot = ox->cy, ob = ox->cy + ox->ch;
        double yc[4] = { ot - t, (ob + SNAP_GAP) - t, ob - b, (ot - SNAP_GAP) - b };
        for (int k = 0; k < 4; k++) if (fabs(yc[k]) < fabs(besty)) { besty = yc[k]; oy = ox; }
    } else if (oy && !ox) {
        double ol = oy->cx, orr = oy->cx + oy->cw;
        double xc[4] = { ol - l, (orr + SNAP_GAP) - l, orr - r, (ol - SNAP_GAP) - r };
        for (int k = 0; k < 4; k++) if (fabs(xc[k]) < fabs(bestx)) { bestx = xc[k]; ox = oy; }
    }
    if (ox) *dx += bestx;
    if (oy) *dy += besty;
}

static void snap_resize(Havel *z, Client *c, int move_left, int move_top,
                        double *l, double *t, double *r, double *b,
                        int *got_x, int *got_y) {
    *got_x = *got_y = 0;
    if (!SNAP_ENABLED) return;
    double bestx = SNAP_DIST, besty = SNAP_DIST;
    int gotx = 0, goty = 0;
    for (int i = 0; i < z->nclients; i++) {
        Client *o = &z->clients[i];
        if (o == c) continue;
        double ol = o->cx, orr = o->cx + o->cw, ot = o->cy, ob = o->cy + o->ch;
        if (*t < ob && *b > ot) {
            double edge = move_left ? *l : *r;
            double cand[2];
            if (move_left) { cand[0] = ol  - edge; cand[1] = (orr + SNAP_GAP) - edge; }
            else           { cand[0] = orr - edge; cand[1] = (ol  - SNAP_GAP) - edge; }
            for (int k = 0; k < 2; k++) if (fabs(cand[k]) < fabs(bestx)) { bestx = cand[k]; gotx = 1; }
        }
        if (*l < orr && *r > ol) {
            double edge = move_top ? *t : *b;
            double cand[2];
            if (move_top) { cand[0] = ot - edge; cand[1] = (ob + SNAP_GAP) - edge; }
            else          { cand[0] = ob - edge; cand[1] = (ot - SNAP_GAP) - edge; }
            for (int k = 0; k < 2; k++) if (fabs(cand[k]) < fabs(besty)) { besty = cand[k]; goty = 1; }
        }
    }
    if (gotx) { if (move_left) *l += bestx; else *r += bestx; }
    if (goty) { if (move_top)  *t += besty; else *b += besty; }
    *got_x = gotx; *got_y = goty;
}

static void size_detent(Havel *z, int move_left, int move_top,
                        double *l, double *t, double *r, double *b,
                        int got_x, int got_y) {
    if (!SIZE_DETENT_ENABLED) return;
    if (!got_x) {
        double w = *r - *l, best = SIZE_DETENT_DIST, target = 0;
        for (int n = 0; n < SIZE_DETENT_LEVELS; n++) {
            double cand = z->sw / (double)(1 << n);
            if (fabs(w - cand) < best) { best = fabs(w - cand); target = cand; }
        }
        if (target > 0) { if (move_left) *l = *r - target; else *r = *l + target; }
    }
    if (!got_y) {
        double h = *b - *t, best = SIZE_DETENT_DIST, target = 0;
        for (int n = 0; n < SIZE_DETENT_LEVELS; n++) {
            double cand = z->sh / (double)(1 << n);
            if (fabs(h - cand) < best) { best = fabs(h - cand); target = cand; }
        }
        if (target > 0) { if (move_top) *t = *b - target; else *b = *t + target; }
    }
}

static void on_motion(Havel *z, XMotionEvent *ev) {
    XEvent next;
    while (XCheckTypedEvent(z->dpy, MotionNotify, &next)) *ev = next.xmotion;
    if (z->moving) {
        Client *c = find(z, z->move_win);
        if (c) {
            double dx = (ev->x_root - z->move_sx) / z->vp.zoom;
            double dy = (ev->y_root - z->move_sy) / z->vp.zoom;
            snap_translate(z, c, z->move_cx, z->move_cy, &dx, &dy);
            c->cx = z->move_cx + dx;
            c->cy = z->move_cy + dy;
            request_redraw(z);
        }
    } else if (z->resizing) {
        Client *c = find(z, z->resize_win);
        if (c) {
            double dx = (ev->x_root - z->resize_sx) / z->vp.zoom;
            double dy = (ev->y_root - z->resize_sy) / z->vp.zoom;
            double l = z->resize_cx0, t = z->resize_cy0;
            double r = l + z->resize_cw0, b = t + z->resize_ch0;
            if (z->resize_left) l += dx; else r += dx;
            if (z->resize_top)  t += dy; else b += dy;
            int got_x, got_y;
            snap_resize(z, c, z->resize_left, z->resize_top, &l, &t, &r, &b, &got_x, &got_y);
            size_detent(z, z->resize_left, z->resize_top, &l, &t, &r, &b, got_x, got_y);
            if (r - l < 20) { if (z->resize_left) l = r - 20; else r = l + 20; }
            if (b - t < 20) { if (z->resize_top)  t = b - 20; else b = t + 20; }
            c->cx = l; c->cy = t;
            c->cw = (int)(r - l); c->ch = (int)(b - t);
            park(z, c);
            request_redraw(z);
        }
    } else if (z->panning) {
        z->vp.cx = z->pan_vx - (ev->x_root - z->pan_sx) / z->vp.zoom;
        z->vp.cy = z->pan_vy - (ev->y_root - z->pan_sy) / z->vp.zoom;
        request_redraw(z);
    }
}

static void on_hover(Havel *z) {
    Window root_ret, child_ret;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    if (!XQueryPointer(z->dpy, z->root, &root_ret, &child_ret,
                        &root_x, &root_y, &win_x, &win_y, &mask))
        return;

    XWindowAttributes a;
    Window ov = hit_override(z, root_x, root_y, &a);
    Client *c = ov ? NULL : hit(z, root_x, root_y);
    if (c && z->hover_win && c->window != z->hover_win) {
        Client *prev = find(z, z->hover_win);
        if (prev) park(z, prev);
    }
    if (!c) {
        if (!ov && z->hover_win) {
            Client *prev = find(z, z->hover_win);
            if (prev) park(z, prev);
            z->hover_win = 0;
        }
        return;
    }

    if (focusfollowsmouse && !z->moving && !z->resizing && !z->panning
            && !(mask & (Button1Mask | Button2Mask | Button3Mask))
            && c->window != z->focused) {
        z->focused = c->window;
        XSetInputFocus(z->dpy, c->window, RevertToPointerRoot, CurrentTime);
        request_redraw(z);
    }
    int wx, wy;
    to_client_local(c, &z->vp, root_x, root_y, &wx, &wy);
    XMoveWindow(z->dpy, c->window, root_x - wx, root_y - wy);
    z->hover_win = c->window;
}
