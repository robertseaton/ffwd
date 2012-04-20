#ifndef VO

#include <libavcodec/avcodec.h>

enum video_output {
     X11
}; 

int draw(AVFrame *frame, enum video_output vo);

#define VO
#endif
