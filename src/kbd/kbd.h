#ifndef KBD

enum kbd {
     KBD_X11
};

void kbd_event_loop(enum kbd k);

#define KBD
#endif
