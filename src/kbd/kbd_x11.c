#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <pthread.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "kbd_defs.h"

/* declared in vo/x11.c */
extern Display *d;
extern Window window;
extern bool initialized;
extern pthread_mutex_t x11mutex;
extern pthread_cond_t initial;
extern int height;
extern int width;

void x11_fullscreen() {
     XEvent xev;
     Atom wm_state = XInternAtom(d, "_NET_WM_STATE", False);
     Atom fullscreen = XInternAtom(d, "_NET_WM_STATE_FULLSCREEN", False);

     memset(&xev, 0, sizeof(xev));
     xev.type = ClientMessage;
     xev.xclient.window = window;
     xev.xclient.message_type = wm_state;
     xev.xclient.format = 32;
     xev.xclient.data.l[0] = 1;
     xev.xclient.data.l[1] = fullscreen;
     xev.xclient.data.l[2] = 0;

     XSendEvent(d, DefaultRootWindow(d), False, SubstructureNotifyMask, &xev);
}

void x11_event_loop() {
     XEvent ev;
     char string[25];
     int len;
     KeySym keysym;

     pthread_mutex_lock(&x11mutex);
     pthread_cond_wait(&initial, &x11mutex);
     pthread_mutex_unlock(&x11mutex);

     XSelectInput(d, window, KeyPressMask | StructureNotifyMask);

     while (true) {
          XNextEvent(d, &ev);

          switch (ev.type) {
          case KeyPress:
               len = XLookupString(&ev.xkey, string, 25, &keysym, NULL);
               if (len < 0)
                    break; 

               if (strcmp(string, KBD_QUIT) == 0) {
                    XCloseDisplay(d);
                    exit(0);
               } else if (strcmp(string, KBD_FULLSCREEN) == 0) {
                    x11_fullscreen();
               }
               break;
          case ConfigureNotify:
               if (ev.xconfigure.window == window) {
                    pthread_mutex_lock(&x11mutex);
                    height = ev.xconfigure.height;
                    width = ev.xconfigure.width;
                    pthread_mutex_unlock(&x11mutex);
               }
               break;
          default:
               break;
          }
     }
}
