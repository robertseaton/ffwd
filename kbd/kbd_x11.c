#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "kbd_defs.h"

/* declared in vo/x11.c */
extern Display *d;
extern Window w;
extern bool initialized;

void x11_event_loop() {
     XEvent ev;
     char string[25];
     int len;
     KeySym keysym;

     while (initialized == false)
          ;

     XSelectInput(d, w, KeyPressMask);

     while (true) {
          XNextEvent(d, &ev);

          if (ev.type == KeyPress) {
               len = XLookupString(&ev.xkey, string, 25, &keysym, NULL);

               if (len < 0)
                    break; 

               if (strcmp(string, KBD_QUIT) == 0) {
                    XCloseDisplay(d);
                    exit(0);
               }
          }
     }
}
