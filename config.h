#ifndef CONFIG_H
#define CONFIG_H

// maximum no. of workspaces
#define MAX_WORKSPACES 9

static float master_size = 0.6; // size of the master window range -> (0, 1)
static unsigned int margin_top = 6;
static unsigned int margin_right = 6;
static unsigned int margin_bottom = 6;
static unsigned int margin_left = 6;
static unsigned int gap = 10; // gap between 2 windows
static char focused_border_color[] = "#ffffff";
static char normal_border_color[] = "#10151a";
static unsigned int border_width = 1;

static const Rule rules[] = {
    /* xprop:
     * WM_CLASS(STRING) = class
     * WM_NAME(STRING) = title
     */

    /* class        title       isfloating */
    {"Gcolor3",     NULL,       true},
};

/* Mod4Mask -> super key
 * Mod1Mask -> Alt key
 * ControlMask -> control key
 * ShiftMask -> shift key
 */

// modifier key
static const unsigned int MODKEY = Mod4Mask;

static const Key keys[] = {
    // quit devoidwm
    {MODKEY|ShiftMask, XK_q, quit, {0}},

    // focus the next/prev window
    {MODKEY, XK_j, focus_adjacent, {.i = 1}},
    {MODKEY, XK_k, focus_adjacent, {.i = -1}},

    // swap slave window with the master window
    {MODKEY, XK_space, zoom, {0}},

    // rotate a window through the stack
    {MODKEY|ShiftMask, XK_j, move_client, {.i = 1}},
    {MODKEY|ShiftMask, XK_k, move_client, {.i = -1}},

    // kill a window
    {MODKEY, XK_x, kill_client, {0}},

    // switch workspaces
    {MODKEY, XK_1, switch_ws, {.i = 0}},
    {MODKEY, XK_2, switch_ws, {.i = 1}},
    {MODKEY, XK_3, switch_ws, {.i = 2}},
    {MODKEY, XK_4, switch_ws, {.i = 3}},
    {MODKEY, XK_5, switch_ws, {.i = 4}},
    {MODKEY, XK_6, switch_ws, {.i = 5}},
    {MODKEY, XK_7, switch_ws, {.i = 6}},
    {MODKEY, XK_8, switch_ws, {.i = 7}},
    {MODKEY, XK_9, switch_ws, {.i = 8}},

    // toggle fullscreen
    {MODKEY|ShiftMask, XK_f, toggle_fullscreen, {0}},

    // change the size of the master window
    {MODKEY, XK_h, change_master_size, {.i = -5}},
    {MODKEY, XK_l, change_master_size, {.i = 5}},

    // send focused client to a different workspace
    {MODKEY|ShiftMask, XK_1, send_to_ws, {.i = 0}},
    {MODKEY|ShiftMask, XK_2, send_to_ws, {.i = 1}},
    {MODKEY|ShiftMask, XK_3, send_to_ws, {.i = 2}},
    {MODKEY|ShiftMask, XK_4, send_to_ws, {.i = 3}},
    {MODKEY|ShiftMask, XK_5, send_to_ws, {.i = 4}},
    {MODKEY|ShiftMask, XK_6, send_to_ws, {.i = 5}},
    {MODKEY|ShiftMask, XK_7, send_to_ws, {.i = 6}},
    {MODKEY|ShiftMask, XK_8, send_to_ws, {.i = 7}},
    {MODKEY|ShiftMask, XK_9, send_to_ws, {.i = 8}},
};

#endif
