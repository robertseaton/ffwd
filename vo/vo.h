#ifndef VO

#include <libavcodec/avcodec.h>

enum video_output {
     X11
}; 

int draw(AVFrame *frame, int vo);

#define VO
#endif
