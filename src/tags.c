#include <X11/Xlib.h>
#include <stdbool.h>

#include "client.h"
#include "dwindle.h"
#include "devoid.h"
#include "tags.h"
#include "focus.h"

void view(Arg arg) {
    if (arg.ui == seltags) return;
    seltags = arg.ui;
    showhide(head);
    tile();
    focus(NULL);
    if (getfullscrlock(seltags)) lock_fullscr(sel);
}

void toggletag(Arg arg) {
    seltags ^= arg.ui;
    showhide(head);
    tile();
}

bool getfullscrlock(unsigned int tags) {
    Client *c;
    for (c = stack; c && !isvisible(c, tags); c = c -> snext);
    if (!c) return 0;
    return c -> isfullscr;
}

void tag(Arg arg) {
    if (arg.ui == sel -> tags) return;
    sel -> tags = arg.ui;
    XMoveWindow(dpy, sel -> win, XDisplayWidth(dpy, screen), XDisplayHeight(dpy, screen));
    if (!sel -> isfloating) tile();
    focus(NULL);
}
