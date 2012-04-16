#include <assert.h>

#include "kbd.h"
#include "kbd_internal.h"

void kbd_event_loop(enum kbd k) {
     switch (k) {
     case KBD_X11:
          x11_event_loop();
          break;
     default:
          assert(1 == 0); /* not implemented */
     }
}
