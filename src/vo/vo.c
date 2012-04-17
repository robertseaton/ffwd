#include <assert.h>

#include "vo.h"
#include "draw.h"

int draw(AVFrame *frame, int vo) {
     switch (vo) {
     case X11:
          if (x11_draw(frame) == -1)
               return -1;
          break;
     default:
          assert(1 == 0); /* not implemented */
     }

     return 0;
}
