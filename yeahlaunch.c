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

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <X11/Xlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sysexits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "yeahlaunch.h"

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
int opt_step = DEF_STEP;
int height;
int raised = 0;
int width = 0;
int drawdir = 0;
int opt_x = 0;
XColor afg, fg, bg, dummy;

struct tab *tabs = NULL;
size_t ntabs = 0;

void setwidths() {
    int scrwid;
    size_t i;
    int rt = 0;

    for (i = 0; i < ntabs; i++) {
        if ((tabs[i].label != NULL) && tabs[i].label[0]) {
            tabs[i].width = XTextWidth(font, tabs[i].label, strlen(tabs[i].label)) + DEF_PADDING;
        } else {
            tabs[i].width = DEF_SEP_WIDTH;
        }
        rt += tabs[i].width;
    }
    width = rt; /* Now everyone has a width. */

    if (drawdir > 0) {
        tabs[0].x = opt_x;
    } else if (drawdir <= 0) {
        scrwid = DisplayWidth(dpy, screen);
        if (drawdir < 0) {
            tabs[0].x = scrwid - opt_x;
        } else {
            tabs[0].x = (scrwid - width) / 2;
        }
    }
    rt = tabs[0].width + tabs[0].x;

    for (i = 0; i < ntabs; i++) {
        tabs[i].x = rt;
        rt += tabs[i].width;
    }
    /* And now everyone is positioned, too. */
}

void make_new_tab(const char *label, const char *cmd) {
    struct tab *t;

    if ((t = realloc(tabs, (ntabs + 1) * sizeof (struct tab))) != NULL) {
        tabs = t;
        tabs[ntabs].label = label[0] ? strdup(label) : NULL;
        tabs[ntabs].cmd = cmd[0] ? strdup(cmd) : NULL;
        ntabs++;
    } else {
        yeah_perror("yeahlaunch: realloc() failed for \"%s\", \"%s\"", label, cmd);
    }
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

void draw_tabs(void) {
    XSetWindowAttributes attrib;
    size_t i = 0;

    attrib.override_redirect = True;
    attrib.background_pixel = bg.pixel;

    setwidths();

    for (i = 0; i < ntabs; i++) {
        tabs[i].win = XCreateWindow(dpy, root, tabs[i].x, 1 - height, tabs[i].width, height,
            0, CopyFromParent, InputOutput, CopyFromParent,
            CWOverrideRedirect | CWBackPixel, &attrib);
        XSelectInput(dpy, tabs[i].win, EnterWindowMask | LeaveWindowMask |
            ButtonPressMask | ButtonReleaseMask | ExposureMask);
        XMapWindow(dpy, tabs[i].win);
    }
}

void hide() {
    size_t i;

    for (i = 0; i < ntabs; i++) {
        XMoveWindow(dpy, tabs[i].win, tabs[i].x, -height + 1);
        /* XLowerWindow(dpy, tabs[i].win); */
    }
    raised = 0;
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

size_t find_tab(Window w) {
    size_t i;

    for (i = 0; i < ntabs; i++) {
        if (w == tabs[i].win) {
            return i;
        }
    }
    return -1;
}

void handle_buttonpress(XEvent event) {
    size_t i;
    int in = 1;

    i = find_tab(event.xbutton.window);
    if (i >= 0) {
        DRAW_LABEL(i, agc);
    }

    do {
        XMaskEvent(dpy, ButtonReleaseMask | LeaveWindowMask | EnterWindowMask,
            &event);
        if (event.type == LeaveNotify) {
            DRAW_LABEL(i, gc);
            in--;
        } else if ((event.type == EnterNotify) &&
            (i == find_tab(event.xcrossing.window))) {
            DRAW_LABEL(i, agc);
            in++;
        }
    } while (event.type != ButtonRelease);

    if (in) {
        spawn(tabs[i].cmd);
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
    DRAW_LABEL(i, gc);
}

int syntax(void) {
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
    return EINVAL;
}

int main(int argc, char *argv[]) {
    XEvent event;
    XGCValues gv;
    int n, m;
    size_t i;

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, " can't open dpy %s", XDisplayName(NULL));
        return EX_UNAVAILABLE;
    }
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    for (n = 1; n < argc; n++) {
        if ((n + 1) < argc) {
            if (argv[n][0] == '-') {
                if (!strcmp(&argv[n][1], "fn")) {
                    SAFE_FREE(opt_font);
                    opt_font = strdup(argv[++n]);
                } else if (!strcmp(&argv[n][1], "fg")) {
                    SAFE_FREE(opt_fg);
                    opt_fg = strdup(argv[++n]);
                } else if (!strcmp(&argv[n][1], "afg")) {
                    SAFE_FREE(opt_afg);
                    opt_afg = strdup(argv[++n]);
                } else if (!strcmp(&argv[n][1], "bg")) {
                    SAFE_FREE(opt_bg);
                    opt_bg = strdup(argv[++n]);
                } else if (!strcmp(&argv[n][1], "x")) {
                    opt_x = atoi(argv[++n]);
                    drawdir = 1;
                } else if (!strcmp(&argv[n][1], "rx")) {
                    opt_x = atoi(argv[++n]);
                    drawdir = -1;
                } else if (!strcmp(&argv[n][1], "step")) {
                    opt_step = atoi(argv[++n]);
                } else if (!strcmp(&argv[n][1], "c")) {
                    conf = argv[++n];
                } else {
                    return syntax();
                }
            } else {
                m = n++;
                make_new_tab(argv[m], argv[n]);
            }
        } else {
            return syntax();
        }
    }

    readrc();

    if (ntabs == 0) {
        return syntax();
    }

    if (opt_font == NULL) {
        opt_font = strdup(DEF_FONT);
    }
    if (opt_fg == NULL) {
        opt_fg = strdup(DEF_FG);
    }
    if (opt_afg == NULL) {
        opt_afg = strdup(DEF_AFG);
    }
    if (opt_bg == NULL) {
        opt_bg = strdup(DEF_BG);
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
                    for (i = 0; i < ntabs; i++) {
                        XRaiseWindow(dpy, tabs[i].win);
                    }
                    raised++;
                }

                /* walk left, staggering nearby tabs */
                for (n = 0, i = find_tab(event.xcrossing.window); (i >= 0) && (i < ntabs) && (n > (1 - height)); i++) {
                    XMoveWindow(dpy, tabs[i].win, tabs[i].x, n);
                    n -= opt_step;
                }

                /* walk right, staggering nearby tabs */
                for (n = 0, i = find_tab(event.xcrossing.window); (i >= 0) && (i < ntabs) && (n > (1 - height)); i--) {
                    XMoveWindow(dpy, tabs[i].win, tabs[i].x, n);
                    n -= opt_step;
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
                    i = find_tab(event.xexpose.window);
                    if ((i >= 0) && (i < ntabs)) {
                        DRAW_LABEL(i, gc);
                        XFlush(dpy);
                    }
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

    for (i = 0; i < ntabs; i++) {
        SAFE_FREE(tabs[i].label);
        SAFE_FREE(tabs[i].cmd);
    }
    free(tabs);
    return 0;
}
