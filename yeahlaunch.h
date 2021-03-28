#ifndef __YEAHLAUNCH_H
#define __YEAHLAUNCH_H

#include <X11/Xlib.h>

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#define DRAW_LABEL(i, gc) { if ((tabs[i].label != NULL) && tabs[i].label[0]) { \
    XDrawString(dpy, tabs[i].win, (gc), 3, height - 4, tabs[i].label, strlen(tabs[i].label)); \
} }
#define SAFE_FREE(x) { if ((x) != NULL) free(x); }
#define RCFILE ".yeahlaunchrc"
#define DEF_STEP 3
#define DEF_SEP_WIDTH 32
#define DEF_PADDING 6

#define DEF_FONT "fixed"
#define DEF_FG "white"
#define DEF_AFG "yellow"
#define DEF_BG "blue"

struct tab {
    char *label;
    char *cmd;
    int x, width;
    Window win;
};

struct config {
    char *fn; /* config file */
    char *fontname;
    char *fg;
    char *afg;
    char *bg;
    unsigned int step;
    /* A negative drawdir is the X interpretation of a negative geometry.  A
    positive drawdir is normal, and zero means center the tabs on the screen. */
    int drawdir;
    unsigned int x;
};

#define yeah_perror(s, a...) fprintf(stderr, __FILE__ "%s: %d: " s ": %s", __func__, __LINE__, a, strerror(errno));

#endif
