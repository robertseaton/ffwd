#ifndef VO

#include <libavcodec/avcodec.h>

enum video_output {
     X11
}; 

int draw(AVFrame *frame, enum video_output vo);
void draw_subtitle(AVSubtitle *sub);

#define VO
#endif
