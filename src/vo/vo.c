#include <assert.h>

#include "vo.h"
#include "vo_internal.h"

int draw(AVFrame *frame, enum video_output vo) {
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
