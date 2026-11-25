/* Havel implementation fragment: mainloop.h
 * Included in order by havel.c; not compiled separately. */


static void setup_ewmh(Havel *h)
{
    Atom check = XInternAtom(
        h->dpy, "_NET_SUPPORTING_WM_CHECK", False
    );
    Atom net_name = XInternAtom(
        h->dpy, "_NET_WM_NAME", False
    );
    Atom utf8 = XInternAtom(
        h->dpy, "UTF8_STRING", False
    );

    h->wm_check = XCreateSimpleWindow(
        h->dpy, h->root, 0, 0, 1, 1, 0, 0, 0
    );

    XChangeProperty(
        h->dpy, h->wm_check, check,
        XA_WINDOW, 32, PropModeReplace,
        (unsigned char *)&h->wm_check, 1
    );

    XStoreName(h->dpy, h->wm_check, "Havel");

    XChangeProperty(
        h->dpy, h->wm_check, net_name,
        utf8, 8, PropModeReplace,
        (const unsigned char *)"Havel", 5
    );

    XChangeProperty(
        h->dpy, h->root, check,
        XA_WINDOW, 32, PropModeReplace,
        (unsigned char *)&h->wm_check, 1
    );

    XSync(h->dpy, False);
}

int main(int argc, char **argv) {
    (void)argc;
    g_argv = argv;
    Havel z = { .vp = { .zoom = 1.0 } };

    if (!(z.dpy = XOpenDisplay(NULL))) { fputs("havel: cannot open display\n", stderr); return 1; }
    int scr = DefaultScreen(z.dpy);
    z.root = RootWindow(z.dpy, scr);
    z.sw = z.vp.sw = DisplayWidth(z.dpy, scr);
    z.sh = z.vp.sh = DisplayHeight(z.dpy, scr);

    XSetErrorHandler(xerr);
    load_state(z.dpy, z.root, &g_saved);
    if (g_saved.valid) { z.vp.cx = g_saved.cx; z.vp.cy = g_saved.cy; z.vp.zoom = g_saved.zoom; }
    else load_viewport(z.dpy, z.root, &z.vp);

    for (int try = 0; ; try++) {
        g_other_wm = 0;
        XSelectInput(z.dpy, z.root, SubstructureRedirectMask|SubstructureNotifyMask|PropertyChangeMask);
        XSync(z.dpy, False);
        if (!g_other_wm) break;
        if (!g_saved.valid || try >= 40) {
            fputs("havel: another window manager is already running\n", stderr);
            cleanup(&z); XCloseDisplay(z.dpy); return 1;
        }
        usleep(50000);
    }

    struct sigaction sa = { .sa_handler = on_sigusr1 };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    int eb, ee;
    if (!XCompositeQueryExtension(z.dpy, &eb, &ee)) { fputs("havel: no XComposite\n", stderr); cleanup(&z); XCloseDisplay(z.dpy); return 1; }
    if (!XDamageQueryExtension(z.dpy, &z.damage_event, &ee)) { fputs("havel: no XDamage\n", stderr); cleanup(&z); XCloseDisplay(z.dpy); return 1; }
    if (!XRenderQueryExtension(z.dpy, &eb, &ee))    { fputs("havel: no XRender\n",    stderr); cleanup(&z); XCloseDisplay(z.dpy); return 1; }
    if (!XFixesQueryExtension(z.dpy, &eb, &ee))     { fputs("havel: no XFixes\n",     stderr); cleanup(&z); XCloseDisplay(z.dpy); return 1; }
    if (!XQueryExtension(z.dpy, "XInputExtension", &z.xi_opcode, &eb, &ee)) {
        fputs("havel: no XInput2\n", stderr); cleanup(&z); XCloseDisplay(z.dpy); return 1;
    }
    { int maj = 2, min = 2; XIQueryVersion(z.dpy, &maj, &min); }
    if (!XPresentQueryExtension(z.dpy, &z.present_opcode, &eb, &ee)) {
        fputs("havel: no XPresent\n", stderr); cleanup(&z); XCloseDisplay(z.dpy); return 1;
    }

    z.wm_protocols = XInternAtom(z.dpy, "WM_PROTOCOLS", False);
    z.wm_delete    = XInternAtom(z.dpy, "WM_DELETE_WINDOW", False);
    z.net_wm_state = XInternAtom(z.dpy, "_NET_WM_STATE", False);
    z.net_wm_fullscreen = XInternAtom(z.dpy, "_NET_WM_STATE_FULLSCREEN", False);
    
    z.current_workspace = 0;
    for (int i = 0; i < MAX_WORKSPACES; i++) {
        z.workspaces[i] = z.vp;
    }
    
    long nworkspaces = MAX_WORKSPACES;
    Atom net_num_desktops = XInternAtom(z.dpy, "_NET_NUMBER_OF_DESKTOPS", False);
    XChangeProperty(z.dpy, z.root, net_num_desktops, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&nworkspaces, 1);
    
    long cur_desktop = 0;
    Atom net_cur_desktop = XInternAtom(z.dpy, "_NET_CURRENT_DESKTOP", False);
    XChangeProperty(z.dpy, z.root, net_cur_desktop, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&cur_desktop, 1);

    update_numlockmask(z.dpy);

    for (int i = 0; i < NBINDINGS; i++) {
        const Key *k = &keys[i];
        grab_key(z.dpy, XKeysymToKeycode(z.dpy, k->keysym), k->mod, z.root);
    }

    unsigned char ximask[(XI_LASTEVENT + 7) / 8] = {0};
    XISetMask(ximask, XI_RawMotion);
    XISetMask(ximask, XI_RawKeyPress);
    XISetMask(ximask, XI_RawKeyRelease);
    XISetMask(ximask, XI_RawButtonPress);
    XISetMask(ximask, XI_RawButtonRelease);
    XIEventMask evmask = { XIAllMasterDevices, sizeof(ximask), ximask };
    XISelectEvents(z.dpy, z.root, &evmask, 1);

    XCompositeRedirectSubwindows(z.dpy, z.root, CompositeRedirectAutomatic);
    z.overlay = XCompositeGetOverlayWindow(z.dpy, z.root);
    if (z.overlay == None) {
        fputs("havel: cannot acquire composite overlay window\n", stderr);
        cleanup(&z); XCloseDisplay(z.dpy); return 1;
    }
    XSelectInput(z.dpy, z.overlay, ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask);
    Cursor cur = XCreateFontCursor(z.dpy, XC_left_ptr);
    XDefineCursor(z.dpy, z.root, cur); XFreeCursor(z.dpy, cur);
    XserverRegion empty = XFixesCreateRegion(z.dpy, NULL, 0);
    XFixesSetWindowShapeRegion(z.dpy, z.overlay, ShapeInput, 0, 0, empty);
    XFixesDestroyRegion(z.dpy, empty);

    XWindowAttributes oa;
    if (!XGetWindowAttributes(z.dpy, z.overlay, &oa)) {
        fputs("havel: cannot query overlay window\n", stderr);
        cleanup(&z); XCloseDisplay(z.dpy); return 1;
    }
    XRenderPictFormat *ofmt = XRenderFindVisualFormat(z.dpy, oa.visual);
    if (!ofmt) {
        fputs("havel: unsupported overlay visual\n", stderr);
        cleanup(&z); XCloseDisplay(z.dpy); return 1;
    }
    for (int i = 0; i < 2; i++) {
        z.backbuf[i] = XCreatePixmap(z.dpy, z.root, z.sw, z.sh, oa.depth);
        z.backbuf_pic[i] = XRenderCreatePicture(z.dpy, z.backbuf[i], ofmt, 0, NULL);
        if (z.backbuf[i] == None || z.backbuf_pic[i] == None) {
            fputs("havel: cannot allocate back buffers\n", stderr);
            cleanup(&z); XCloseDisplay(z.dpy); return 1;
        }
    }
    XPresentSelectInput(z.dpy, z.overlay, PresentIdleNotifyMask);

    setup_ewmh(&z);

    Window dummy, *ch = NULL; unsigned int n = 0;
    XQueryTree(z.dpy, z.root, &dummy, &dummy, &ch, &n);
    for (unsigned i = 0; i < n; i++) {
        XWindowAttributes a;
        if (ch[i] != z.overlay && XGetWindowAttributes(z.dpy, ch[i], &a)
                && a.map_state == IsViewable) {
            if (a.override_redirect) manage_override(&z, ch[i]);
            else                      manage(&z, ch[i]);
        }
    }
    if (ch) XFree(ch);

    if (g_saved.valid) {
        z.current_workspace = g_saved.current_workspace;
        for (int i = 0; i < MAX_WORKSPACES; i++) {
            z.workspaces[i] = g_saved.workspaces[i];
            /* Screen dimensions are not part of the saved viewport. */
            z.workspaces[i].sw = z.sw;
            z.workspaces[i].sh = z.sh;
        }
        z.vp = z.workspaces[z.current_workspace];

        long cur_desktop_restored = z.current_workspace;
        XChangeProperty(z.dpy, z.root, net_cur_desktop, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char *)&cur_desktop_restored, 1);
        
        for (int i = 0; i < z.nclients; i++) {
            Client *c = &z.clients[i];
            int found = 0;
            for (int k = 0; k < g_saved.n && !found; k++)
                if (g_saved.c[k].w == c->window) {
                    c->cx = g_saved.c[k].cx; c->cy = g_saved.c[k].cy; 
                    c->workspace = g_saved.c[k].workspace;
                    found = 1;
                }
            if (!found && !c->restored) { c->cx = z.vp.cx - c->cw / 2.0; c->cy = z.vp.cy - c->ch / 2.0; }
        }
        Client ordered[MAX_CLIENTS];
        int nordered = 0;
        for (int k = 0; k < g_saved.n && nordered < MAX_CLIENTS; k++) {
            for (int i = 0; i < z.nclients; i++) {
                if (z.clients[i].window != g_saved.c[k].w) continue;
                int seen = 0;
                for (int j = 0; j < nordered; j++)
                    if (ordered[j].window == z.clients[i].window) { seen = 1; break; }
                if (!seen) ordered[nordered++] = z.clients[i];
                break;
            }
        }
        for (int i = 0; i < z.nclients && nordered < MAX_CLIENTS; i++) {
            int seen = 0;
            for (int k = 0; k < nordered; k++)
                if (ordered[k].window == z.clients[i].window) { seen = 1; break; }
            if (!seen) ordered[nordered++] = z.clients[i];
        }
        if (nordered == z.nclients)
            memcpy(z.clients, ordered, nordered * sizeof *ordered);
        for (int i = 0; i < nordered; i++) XRaiseWindow(z.dpy, z.clients[i].window);
        if (g_saved.focused && find(&z, g_saved.focused)) {
            z.focused = g_saved.focused;
            XSetInputFocus(z.dpy, z.focused, RevertToPointerRoot, CurrentTime);
        }
        XDeleteProperty(z.dpy, z.root, XInternAtom(z.dpy, STATE_PROP, False));
    }
    request_redraw(&z);
    z.last_input = now_secs();

    XEvent ev;
    for (;;) {
        int fd = ConnectionNumber(z.dpy);
        while (!XPending(z.dpy)) {
            if (g_restart_req) restart(&z);
            double wait = 60.0;
            if (IDLE_LOCK_TIMEOUT > 0) {
                double idle = now_secs() - z.last_input;
                if (!z.idle_fired && idle >= IDLE_LOCK_TIMEOUT) {
                    int lock_rc = system(IDLE_LOCK_CMD);
                    (void)lock_rc;
                    z.idle_fired = 1;
                }
                if (!z.idle_fired) wait = IDLE_LOCK_TIMEOUT - idle;
                if (wait < 0) wait = 0;
            }
            struct timeval tv = { (long)wait, (long)((wait - (long)wait) * 1e6) };
            fd_set fds; FD_ZERO(&fds); FD_SET(fd, &fds);
            select(fd + 1, &fds, NULL, NULL, &tv);
        }
        XNextEvent(z.dpy, &ev);
        if (ev.type == z.damage_event + XDamageNotify) {
            XDamageSubtract(z.dpy, ((XDamageNotifyEvent *)&ev)->damage, None, None);
            request_redraw(&z);
            continue;
        }
        switch (ev.type) {
        case PropertyNotify:
            if (ev.xproperty.window == z.root) {
                Atom rp = XInternAtom(z.dpy, "_XROOTPMAP_ID", False);
                Atom ep = XInternAtom(z.dpy, "ESETROOT_PMAP_ID", False);
                if (ev.xproperty.atom == rp || ev.xproperty.atom == ep)
                    request_redraw(&z);
            }
            break;
        case MapRequest:       on_map(&z, &ev.xmaprequest);             break;
        case MapNotify:        on_mapnotify(&z, &ev.xmap);              break;
        case ClientMessage:
            if (ev.xclient.message_type == z.net_wm_state) {
                if (ev.xclient.data.l[1] == (long)z.net_wm_fullscreen ||
                    ev.xclient.data.l[2] == (long)z.net_wm_fullscreen) {
                    Client *c = find(&z, ev.xclient.window);
                    if (c) {
                        long action = ev.xclient.data.l[0]; /* 0 remove, 1 add, 2 toggle */
                        int want = action == 1 ? 1 : action == 0 ? 0 : !c->isfullscreen;
                        if (want != c->isfullscreen) {
                            z.focused = c->window;
                            XSetInputFocus(z.dpy, c->window, RevertToPointerRoot, CurrentTime);
                            fullscreen_toggle(&z, NULL);
                        }
                    }
                }
            }
            break;
        case UnmapNotify:      on_unmap(&z, &ev.xunmap);                break;
        case DestroyNotify:    on_destroy(&z, &ev.xdestroywindow);      break;
        case ConfigureRequest: on_configure(&z, &ev.xconfigurerequest); break;
        case MappingNotify:
            XRefreshKeyboardMapping(&ev.xmapping);
            if (ev.xmapping.request == MappingKeyboard ||
                ev.xmapping.request == MappingModifier) {
                update_numlockmask(z.dpy);
                XUngrabKey(z.dpy, AnyKey, AnyModifier, z.root);
                for (int i = 0; i < NBINDINGS; i++) {
                    const Key *k = &keys[i];
                    grab_key(z.dpy, XKeysymToKeycode(z.dpy, k->keysym), k->mod, z.root);
                }
            }
            break;
        case ButtonPress:      mark_activity(&z); on_button(&z, &ev.xbutton);   break;
        case ButtonRelease:    mark_activity(&z); on_release(&z, &ev.xbutton);  break;
        case MotionNotify:     mark_activity(&z); on_motion(&z, &ev.xmotion);   break;
        case KeyPress:         mark_activity(&z); on_key(&z, &ev.xkey); request_redraw(&z); break;
        case GenericEvent:
            if (ev.xcookie.extension == z.xi_opcode && XGetEventData(z.dpy, &ev.xcookie)) {
                if (ev.xcookie.evtype == XI_RawMotion) {
                    mark_activity(&z); on_hover(&z);
                } else if (ev.xcookie.evtype == XI_RawKeyPress ||
                           ev.xcookie.evtype == XI_RawKeyRelease) {
                    mark_activity(&z); sync_gesture_grab(&z);
                } else if (ev.xcookie.evtype == XI_RawButtonPress ||
                           ev.xcookie.evtype == XI_RawButtonRelease) {
                    /* Refresh the hover/drag target immediately, especially
                     * after a drag ends outside the client's bounds. */
                    mark_activity(&z); on_hover(&z);
                }
                XFreeEventData(z.dpy, &ev.xcookie);
            } else if (ev.xcookie.extension == z.present_opcode && XGetEventData(z.dpy, &ev.xcookie)) {
                if (ev.xcookie.evtype == PresentIdleNotify)
                    on_present_idle(&z, (XPresentIdleNotifyEvent *)ev.xcookie.data);
                XFreeEventData(z.dpy, &ev.xcookie);
            }
            break;
        }
    }
}
