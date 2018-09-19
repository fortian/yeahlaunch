/*  _____________                   Copyright (c) knorke@phrat.de
   < yeahlaunch  >                  Copyright (c) 2009, 2017 bstern@bstern.org
    -------------
           \   ^__^                 This program is free software; you can
            \  (oo)\_______         redistribute it and/or modify it under the
               (__)\       )\/\     terms of the GNU General Public License as
                   ||----w |        published by the Free Software Foundation;
                   ||     ||        either version 2 of the License, or (at your
                                    option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.
*/

#include <X11/Xlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sysexits.h>

#define DRAW_LABEL(gc) { if ((t->label != NULL) && t->label[0]) { \
    XDrawString(dpy, t->win, (gc), 3, height - 4, t->label, strlen(t->label)); \
} }
#define SAFE_FREE(x) { if ((x) != NULL) free(x); }
#define RCFILE ".yeahlaunchrc"
#define STEP_DEFAULT 3
#define DEF_SEP_WIDTH 32

Display *dpy;
Window root;
int screen;
XFontStruct *font;
GC agc, gc, hgc;
char *conf = RCFILE;
char *opt_font = NULL;
char *opt_fg = NULL;
char *opt_afg = NULL;
char *opt_bg = NULL;
int opt_step = STEP_DEFAULT;
int height;
int raised = 0;
int width = 0;
int drawdir = 0;
/* A negative drawdir is the X interpretation of a negative geometry.  A
positive drawdir is "normal", and zero means center the tabs on the screen. */
int opt_x = 0;
XColor afg, fg, bg, dummy;
typedef struct tab tab;
struct tab {
    tab *prev;
    tab *next;
    char *label;
    char *cmd;
    int x, width;
    Window win;
};
tab *head_tab = NULL;
tab *tail_tab = NULL;

void make_new_tab(const char *cmd, const char *label);
tab *find_tab(Window w);
void spawn(char *cmd);
void handle_buttonpress(XEvent event);
void hide(void);
void draw_tabs(void);
void readrc(void);
void setwidths(tab *t);

int main(int argc, char *argv[]) {
    XEvent event;
    XGCValues gv;
    int i;
    tab *t;

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, " can't open dpy %s", XDisplayName(NULL));
        return EX_UNAVAILABLE;
    }
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (!strcmp(&argv[i][1], "fn") && (i + 1 < argc)) {
                SAFE_FREE(opt_font);
                opt_font = strdup(argv[++i]);
            } else if (!strcmp(&argv[i][1], "fg") && (i + 1 < argc)) {
                SAFE_FREE(opt_fg);
                opt_fg = strdup(argv[++i]);
            } else if (!strcmp(&argv[i][1], "afg") && (i + 1 < argc)) {
                SAFE_FREE(opt_afg);
                opt_afg = strdup(argv[++i]);
            } else if (!strcmp(&argv[i][1], "bg") && (i + 1 < argc)) {
                SAFE_FREE(opt_bg);
                opt_bg = strdup(argv[++i]);
            } else if (!strcmp(&argv[i][1], "x") && (i + 1 < argc)) {
                opt_x = atoi(argv[++i]);
                drawdir = 1;
            } else if (!strcmp(&argv[i][1], "rx") && (i + 1 < argc)) {
                opt_x = atoi(argv[++i]);
                drawdir = -1;
            } else if (!strcmp(&argv[i][1], "step") && (i + 1 < argc)) {
                opt_step = atoi(argv[++i]);
            } else if (!strcmp(&argv[i][1], "c") && (i + 1 < argc)) {
                conf = argv[++i];
            } else {
                puts("usage: yeahlaunch label1 cmd1 label2 cmd2 ...\n"
                    "options:\n"
                    "\t-fg color\n"
                    "\t-afg color\n"
                    "\t-bg color\n"
                    "\t-fn font name\n"
                    "\t-x xoffset\n"
                    "\t-rx xoffset [right aligned]\n"
                    "\t-step step size\n"
                    "\t-c config file");
                exit(0);
            }
        } else {
            make_new_tab(argv[i], argv[i + 1]);
            i++;
        }
    }

    readrc();

    if (opt_font == NULL) {
        opt_font = strdup("fixed");
    }
    if (opt_fg == NULL) {
        opt_fg = strdup("white");
    }
    if (opt_afg == NULL) {
        opt_afg = strdup("yellow");
    }
    if (opt_bg == NULL) {
        opt_bg = strdup("black");
    }

    font = XLoadQueryFont(dpy, opt_font);
    height = font->/* max_bounds. */ascent + font->/* max_bounds. */descent + 3;
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_fg, &fg, &dummy);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_afg, &afg, &dummy);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_bg, &bg, &dummy);

    font = XLoadQueryFont(dpy, opt_font);
    height = font->/* max_bounds. */ascent + font->/* max_bounds. */descent + 3;
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_fg, &fg, &dummy);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_afg, &afg, &dummy);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_bg, &bg, &dummy);

    gv.font = font->fid;
    gv.foreground = fg.pixel;
    gv.function = GXcopy;
    gc = XCreateGC(dpy, root, GCFunction | GCForeground | GCFont, &gv);
    gv.foreground = afg.pixel;
    agc = XCreateGC(dpy, root, GCFunction | GCForeground | GCFont, &gv);

    draw_tabs();

    for (;;) {
        XNextEvent(dpy, &event);
        switch (event.type) {
            case EnterNotify:
                if (!raised) {
                    for (t = head_tab; t; t = t->next) {
                        XRaiseWindow(dpy, t->win);
                    }
                    raised++;
                }

                /* walk left, staggering nearby tabs */
                for (i = 0, t = find_tab(event.xcrossing.window);
                    (t != NULL) && (i > (1 - height)); t = t->next) {
                    XMoveWindow(dpy, t->win, t->x, i);
                    i -= opt_step;
                }

                /* walk right, staggering nearby tabs */
                for (i = 0, t = find_tab(event.xcrossing.window);
                    (t != NULL) && (i > (1 - height)); t = t->prev) {
                    XMoveWindow(dpy, t->win, t->x, i);
                    i -= opt_step;
                }
            break;
            case LeaveNotify:
                if ((event.xcrossing.y >= height - opt_step) ||
                    (event.xcrossing.x_root <= opt_x) ||
                    (event.xcrossing.x_root >= opt_x + width)) {
                    hide();
                }
            break;
            case Expose:
                if (event.xexpose.count == 0) {
                    t = find_tab(event.xexpose.window);
                    DRAW_LABEL(gc);
                    XFlush(dpy);
                }
            break;
            case ButtonPress:
                if ((event.xbutton.button == Button1) ||
                    (event.xbutton.button == Button3)) {
                    handle_buttonpress(event);
                }
            break;
        }
    }
    SAFE_FREE(opt_font);
    SAFE_FREE(opt_fg);
    SAFE_FREE(opt_afg);
    SAFE_FREE(opt_bg);

    for (t = head_tab; t != NULL; t = t->next) {
        SAFE_FREE(t->prev);
        SAFE_FREE(t->label);
        SAFE_FREE(t->cmd);
        if (t->next == NULL) {
            free(t);
            break;
        }
    }
    return 0;
}

void draw_tabs(void) {
    XSetWindowAttributes attrib;
    attrib.override_redirect = True;
    attrib.background_pixel = bg.pixel;
    tab *t;

    setwidths(tail_tab);
    for (t = head_tab; t != NULL; t = t->next) {
        t->win = XCreateWindow(dpy, root, t->x, 1 - height, t->width, height,
            0, CopyFromParent, InputOutput, CopyFromParent,
            CWOverrideRedirect | CWBackPixel, &attrib);
        XSelectInput(dpy, t->win, EnterWindowMask | LeaveWindowMask |
            ButtonPressMask | ButtonReleaseMask | ExposureMask);
        XMapWindow(dpy, t->win);
    }
}

void make_new_tab(const char *label, const char *cmd) {
    tab *t;

    if ((t = (tab *)malloc(sizeof (tab))) != NULL) {
        if (head_tab == NULL) {
            t->next = t->prev = NULL;
            tail_tab = t;
        } else {
            t->next = head_tab;
            t->prev = NULL;
            head_tab->prev = t;
        }
        head_tab = t;
        t->label = label[0] ? strdup(label) : NULL;
        t->cmd = cmd[0] ? strdup(cmd) : NULL;
    } else {
        perror("yeahlaunch: malloc() failed");
    }
}

void setwidths(tab *t) {
    int scrwid;
    tab *h;
    int rt = 0;

    for (h = t; h != NULL; h = h->prev) {
        if ((h->label != NULL) && h->label[0]) {
            h->width = XTextWidth(font, h->label, strlen(h->label)) + 6;
        } else {
            h->width = DEF_SEP_WIDTH;
        }
        rt += h->width;
    }
    width = rt; /* Now everyone has a width. */

    if (drawdir > 0) {
        t->x = opt_x;
    } else if (drawdir <= 0) {
        scrwid = DisplayWidth(dpy, screen);
        if (drawdir < 0) {
            t->x = scrwid - opt_x;
        } else {
            t->x = (scrwid - width) / 2;
        }
    }
    rt = t->width + t->x;

    for (h = t->prev; h != NULL; h = h->prev) {
        h->x = rt;
        rt += h->width;
    }
    /* And now everyone is positioned, too. */
}

tab *find_tab(Window w) {
    tab *t;

    for (t = head_tab; t; t = t->next) {
        if (w == t->win) {
            return t;
        }
    }
    return NULL;
}

void spawn(char *cmd) {
    pid_t pid;

    if ((cmd != NULL) && cmd[0]) {
        pid = fork();
        if (pid < 0) {
            perror("yeahlaunch: fork");
        } else if (!pid) {
            setsid();
            pid = fork();
            if (pid < 0) {
                perror("yeahlaunch child: fork");
            } else if (!pid) {
                execlp("/bin/sh", "sh", "-c", cmd, NULL);
            } else {
                _exit(0);
            }
        } else {
            wait(NULL);
        }
    }
}

void handle_buttonpress(XEvent event) {
    tab *t;
    int in = 1;

    t = find_tab(event.xbutton.window);
    DRAW_LABEL(agc);

    do {
        XMaskEvent(dpy, ButtonReleaseMask | LeaveWindowMask | EnterWindowMask,
            &event);
        if (event.type == LeaveNotify) {
            DRAW_LABEL(gc);
            in--;
        } else if ((event.type == EnterNotify) &&
            (find_tab(event.xcrossing.window) == t)) {
            DRAW_LABEL(agc);
            in++;
        }
    } while (event.type != ButtonRelease);
    if (in) {
        spawn(t->cmd);
#ifdef FLASH_CMD
        for (in = 0; in < 3; in++) {
           DRAW_LABEL(agc);
           XSync(dpy, False);
           usleep(10000);
           DRAW_LABEL(gc);
           XSync(dpy, False);
           usleep(10000); 
        }
#endif
        if (event.xbutton.button == Button3) {
            hide();
        }
    }
    DRAW_LABEL(gc);
}

void hide() {
    tab *t;

    for (t = head_tab; t; t = t->next) {
        XMoveWindow(dpy, t->win, t->x, -height + 1);
        /* XLowerWindow(dpy, t->win); */
    }
    raised = 0;
}

void readrc(void) {
    char *home;
    FILE *f;
    char buf[BUFSIZ];
    char *idx;
    int line = 0;

    if ((home = getenv("HOME")) != NULL) {
        if (chdir(home) < 0) {
            fprintf(stderr, "Warning: could not cd to %s: %s\n",
                home, strerror(errno));
        }
    }

    if ((f = fopen(conf, "r")) != NULL) {
        while (fgets(buf, BUFSIZ, f) != NULL) {
            line++;
            buf[BUFSIZ - 1] = 0;
            if ((idx = strchr(buf, '#')) != NULL) {
                *idx = 0;
            }
            if ((idx = strchr(buf, '\n')) != NULL) {
                *idx = 0;
            }
            if (!buf[0]) {
                continue;
            }

#ifdef DEBUG
            printf("Found line %d: `%s'\n", line, buf);
            fflush(stdout);
#endif

            if ((idx = strchr(buf, '=')) == NULL) {
                fprintf(stderr, "Syntax error on line %d of %s: `%s'\n",
                    line, conf, buf);
                continue;
            }
            *idx = 0;

            if (buf[0] == '-') {
                if (!strcmp(&buf[1], "fn")) {
                    SAFE_FREE(opt_font);
                    opt_font = strdup(idx + 1);
                } else if (!strcmp(&buf[1], "fg")) {
                    SAFE_FREE(opt_fg);
                    opt_fg = strdup(idx + 1);
                } else if (!strcmp(&buf[1], "afg")) {
                    SAFE_FREE(opt_afg);
                    opt_afg = strdup(idx + 1);
                } else if (!strcmp(&buf[1], "bg")) {
                    SAFE_FREE(opt_bg);
                    opt_bg = strdup(idx + 1);
                } else if (!strcmp(&buf[1], "x")) {
                    opt_x = atoi(idx + 1);
                    drawdir = 1;
                } else if (!strcmp(&buf[1], "rx")) {
                    opt_x = atoi(idx + 1);
                    drawdir = -1;
                } else if (!strcmp(&buf[1], "step")) {
                    opt_step = atoi(idx + 1);
                } else {
                    fprintf(stderr,
                        "Ignoring unknown option on line %d of %s: `%s=%s'\n",
                        line, conf, buf, idx + 1);
                }
            } else {
                make_new_tab(buf, idx + 1);
            }
        }
        fclose(f);
    } else if (errno != ENOENT) { /* It's okay if they don't have one. */
        fprintf(stderr, "Warning: could not fopen(%s): %s\n",
            conf, strerror(errno));
    }
}
