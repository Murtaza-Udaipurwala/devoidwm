#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xutil.h>

#define MAX(a, b) (a) > (b) ? (a) : (b)
#define MODCLEAN(mask) (mask & \
        (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define MAX_WORKSPACES 9

typedef struct Client Client;
struct Client {
    unsigned int x, y, old_x, old_y;
    unsigned int width, height, old_width, old_height;
    Window win;
    Client *next;
    Client *prev;
    bool isfloating, isfullscreen;
};

static void start(void);
static void stop(void);
static void loop(void);
static void getInput(void);
static void handleKeyPress(XEvent *event);
static void handleButtonPress(XEvent *event);
static void handlePointerMotion(XEvent *event);
static void handleButtonRelease(XEvent *event);
static void map(XEvent *event);
static void configureWindow(Client *client);
static void quit(XEvent *event, char *command);
static void focusAdjacent(XEvent *event, char *command);
static void kill(XEvent *event, char *command);
static void destroyNotify(XEvent *event);
static void restack();
static void configureSlaveWindows(Client *firstSlave, unsigned int slaveCount);
static void zoom(XEvent *event, char *command);
static void swap(Client *focusedClient, Client *targetClient);
static void swapWithNeighbour(XEvent *event, char *command);
static void focus(Client *client);
static void setClientRules(Client *client);
static void switchWorkspace(XEvent *event, char *command);
static void saveWorkspace(Client *focused, Client *head, unsigned int totalClients, unsigned int ws);

static bool running;
static Display *dpy;
static XButtonEvent prevPointerPosition;
static XWindowAttributes attr;

struct root {
    Window win;
    int width;
    int height;
} root;

static Client *head = NULL;
static Client *focused = NULL;
static unsigned int currentWorkspace = 0;
static unsigned int totalClients = 0;

struct workspace {
    Client *head;
    Client *focused;
    unsigned int totalClients;
};

static struct workspace workspaces[MAX_WORKSPACES];

static const unsigned int MODKEY = Mod4Mask;

typedef struct {
    unsigned int modifier;
    KeySym keysym;
    void (*execute)(XEvent *event, char *command);
    char *command;
} key;

static const key keys[] = {
    {MODKEY|ShiftMask, XK_q, quit, NULL},
    {MODKEY, XK_j, focusAdjacent, "next"},
    {MODKEY, XK_k, focusAdjacent, "prev"},
    {MODKEY, XK_x, kill, NULL},
    {MODKEY, XK_space, zoom, NULL},
    {MODKEY|ShiftMask, XK_j, swapWithNeighbour, "next"},
    {MODKEY|ShiftMask, XK_k, swapWithNeighbour, "prev"},
    {MODKEY, XK_1, switchWorkspace, "0"},
    {MODKEY, XK_2, switchWorkspace, "1"},
    {MODKEY, XK_3, switchWorkspace, "2"},
    {MODKEY, XK_4, switchWorkspace, "3"},
    {MODKEY, XK_5, switchWorkspace, "4"},
    {MODKEY, XK_6, switchWorkspace, "5"},
    {MODKEY, XK_7, switchWorkspace, "6"},
    {MODKEY, XK_8, switchWorkspace, "7"},
    {MODKEY, XK_9, switchWorkspace, "8"},
};

typedef struct Rules {
    char *class;
    bool isfloating;
} Rules;

static const Rules rules[] = {
    {"Pinentry-gtk-2", true},
};

static void (*handleEvents[LASTEvent])(XEvent *event) = {
    [KeyPress] = handleKeyPress,
    [ButtonPress] = handleButtonPress,
    [ButtonRelease] = handleButtonRelease,
    [MotionNotify] = handlePointerMotion,
    [MapRequest] = map,
    [DestroyNotify] = destroyNotify,
};

void saveWorkspace(Client *focused, Client *head,unsigned int totalClients, unsigned int ws) {
    workspaces[ws].focused = focused;
    workspaces[ws].head = head;
    workspaces[ws].totalClients = totalClients;
}

void switchWorkspace(XEvent *event, char *command) {
    (void)event;
    unsigned int ws = (int)(command[0] - '0');
    if (ws == currentWorkspace) return;

    saveWorkspace(focused, head, totalClients, currentWorkspace);

    currentWorkspace = ws;
    Client *client = focused;
    if (client != NULL) {
        do {
            XUnmapWindow(dpy, client -> win);
            client = client -> next;
        } while (client != NULL && client != focused);
    }

    focused = workspaces[ws].focused;
    head = workspaces[ws].head;
    totalClients = workspaces[ws].totalClients;

    if (focused != NULL) {
        client = focused;
        do {
            XMapWindow(dpy, client -> win);
            client = client -> next;
        } while (client != NULL && client != focused);
    }
    focus(focused);
}

void setClientRules(Client *client) {
    client -> isfloating = false;

    XClassHint hint;
    XGetClassHint(dpy, client -> win, &hint);

    for (size_t i = 0; i < sizeof(rules)/ sizeof(Rules); i ++) {
        if (strcmp(hint.res_class, rules[i].class) == 0) {
            client -> isfloating = rules[i].isfloating;
            break;
        };
    }
}

void focus(Client *client) {
    focused = client;
    if (focused == NULL) return;
    XSetInputFocus(dpy, focused -> win, RevertToParent, CurrentTime);
    XRaiseWindow(dpy, focused -> win);
}

void swapWithNeighbour(XEvent *event, char *command) {
    (void)event;
    if (focused == NULL || focused -> next == NULL || focused -> isfloating)
        return;
    swap(focused, command[0] == 'n' ? focused -> next : focused -> prev);
}

void swap(Client *focusedClient, Client *targetClient) {
    Window temp = focusedClient -> win;
    focusedClient -> win = targetClient -> win;
    targetClient -> win = temp;

    configureWindow(focusedClient);
    configureWindow(targetClient);
    focus(targetClient);
}

void zoom(XEvent *event, char *command) {
    (void)command;
    (void)event;
    if (focused == NULL || focused -> next == NULL || focused -> isfloating)
        return;
    swap(head, focused);
    focus(head);
}

void configureSlaveWindows(Client *firstSlave, unsigned int slaveCount) {
    for (unsigned int i = 0; i < slaveCount; i ++) {
        firstSlave -> x = root.width / 2;
        firstSlave -> y = (i * root.height) / slaveCount;
        firstSlave -> width = root.width / 2;
        firstSlave -> height = root.height / slaveCount;
        configureWindow(firstSlave);
        firstSlave = firstSlave -> next;
    }
}

void restack() {
    if (head == NULL) return;
    head -> x = 0;
    head -> y = 0;
    head -> height = root.height;
    if (totalClients == 1) head -> width = root.width;
    else {
        head -> width = root.width / 2;
        configureSlaveWindows(head -> next, totalClients - 1);
    }
    configureWindow(head);
}

void destroyNotify(XEvent *event) {
    Window destroyedWindow = event -> xdestroywindow.window;
    Client *client = head;

    do {
        if (client -> win == destroyedWindow) {
            if (client == head) head = head -> next;
            if (client -> next == NULL) break;
            if (client -> next -> next == client) {
                client -> next -> next = NULL;
                client -> next -> prev = NULL;
            } else {
                client -> prev -> next = client -> next;
                client -> next -> prev = client -> prev;
            }
            break;
        }
        client = client -> next;
    } while (client != NULL && client != head);

    if (client == NULL) {
        focused = NULL;
        return;
    };

    if (client == head) {
        free(focused);
        focus(head);
        return;
    }

    focus(client -> prev);
    free(client);
    totalClients --;
    restack();
}

void kill(XEvent *event, char *command) {
    (void)command;
    (void)event;
    XSetCloseDownMode(dpy, DestroyAll);
    XKillClient(dpy, focused -> win);
}

void focusAdjacent(XEvent *event, char *command) {
    (void)event;
    if (focused == NULL || focused -> next == NULL || focused -> isfloating)
        return;

    focused = (command[0] == 'n') ? focused -> next : focused -> prev;
    focus(focused);
}

void quit(XEvent *event, char *command) {
    (void)event;
    (void)command;
    running = false;
}

void configureWindow(Client *client) {
    XWindowChanges changes = {
        .x = client -> x,
        .y = client -> y,
        .width = client -> width,
        .height = client -> height
    };
    XConfigureWindow(dpy, client -> win, CWX|CWY|CWWidth|CWHeight, &changes);
}

void map(XEvent *event) {
    Client *newClient = (Client *)malloc(sizeof(Client));
    newClient -> win = event -> xmaprequest.window;
    XMapWindow(dpy, newClient -> win);
    XSelectInput(dpy, newClient -> win, StructureNotifyMask);
    focus(newClient);

    setClientRules(newClient);
    if (newClient -> isfloating) {
        XGetWindowAttributes(dpy, newClient -> win, &attr);
        newClient -> width = attr.width;
        newClient -> height = attr.height;
        newClient -> x = root.width/2 - newClient -> width/2;
        newClient -> y = root.height/2 - newClient -> height/2;
        configureWindow(newClient);
        return;
    };

    newClient -> x = 0;
    newClient -> y = 0;
    newClient -> height = root.height;
    newClient -> next = head;

    if (head == NULL) {
        newClient -> prev = NULL;
        newClient -> width = root.width;
    } else {
        configureSlaveWindows(head, totalClients);
        newClient -> width = root.width / 2;
        if (head -> prev == NULL) {
            newClient -> prev = head;
            head -> next = newClient;
            head -> prev = newClient;
        } else {
            newClient -> prev = head -> prev;
            head -> prev -> next = newClient;
            head -> prev = newClient;
        }
    }

    head = newClient;
    totalClients ++;
    configureWindow(newClient);
}

void handleButtonPress(XEvent *event) {
    if(event -> xbutton.subwindow == None) return;
    XGrabPointer(
        dpy,
        event -> xbutton.subwindow,
        True,
        PointerMotionMask|ButtonReleaseMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None,
        CurrentTime
    );
    XGetWindowAttributes(dpy, event -> xbutton.subwindow, &attr);
    prevPointerPosition = event -> xbutton;
}

void handlePointerMotion(XEvent *event) {
    while(XCheckTypedEvent(dpy, MotionNotify, event));
    int dx = event -> xbutton.x_root - prevPointerPosition.x_root;
    int dy = event -> xbutton.y_root - prevPointerPosition.y_root;
    bool isLeftClick = prevPointerPosition.button == 1;
    XMoveResizeWindow(
        dpy,
        event -> xmotion.window,
        attr.x + (isLeftClick ? dx : 0),
        attr.y + (isLeftClick ? dy : 0),
        MAX(5, attr.width + (isLeftClick ? 0 : dx)),
        MAX(5, attr.height + (isLeftClick ? 0 : dy))
    );
}

void handleButtonRelease(XEvent *event) {
    (void)event;
    XUngrabPointer(dpy, CurrentTime);
}

void handleKeyPress(XEvent *event) {
    for (size_t i = 0; i < sizeof(keys) / sizeof(key); i ++) {
        KeySym keysym = XkbKeycodeToKeysym(dpy, event -> xkey.keycode, 0, 0);
        if (keysym == keys[i].keysym
                && MODCLEAN(keys[i].modifier) == MODCLEAN(event -> xkey.state))
            keys[i].execute(event, keys[i].command);
    }
}

void getInput(void) {
    for (size_t i = 0; i < sizeof(keys) / sizeof(key); i ++)
        XGrabKey(
            dpy,
            XKeysymToKeycode(dpy, keys[i].keysym),
            keys[i].modifier,
            root.win,
            True,
            GrabModeAsync,
            GrabModeAsync
        );

    XGrabButton(dpy, 1, MODKEY, root.win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(dpy, 3, MODKEY, root.win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
}

void loop(void) {
    XEvent event;
    while (running) {
        XNextEvent(dpy, &event); // blocking -> waits for next event to occur
        if (handleEvents[event.type])
            handleEvents[event.type](&event);
    }
}

void start(void) {
    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Failed to establish a connection with the xserver\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Connected to the xserver\n");
    running = true;

    // root window
    root.win = DefaultRootWindow(dpy);
    XGetWindowAttributes(dpy, root.win, &attr);
    root.width = attr.width;
    root.height = attr.height;

    XSelectInput(dpy, root.win, SubstructureRedirectMask);

    for (unsigned int i = 0; i < MAX_WORKSPACES; i ++) {
        workspaces[i].head = NULL;
        workspaces[i].focused= NULL;
        workspaces[i].totalClients = 0;
    }
}

void stop(void) {
    XCloseDisplay(dpy);
    fprintf(stdout, "Disconnected from the xserver\n");
}

int main(void) {
    start();
    getInput();
    loop();
    stop();
    return 0;
}
