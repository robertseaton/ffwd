#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include <X11/Xlib.h>

#include <stdbool.h>

Display *d;
Window w;
GC gc;
XImage *ximg;
bool initialized = false;

XImage *get_ximg(Visual *v, int width, int height) {
     XImage *ximg;

     ximg = XCreateImage(d, v, 24, ZPixmap, 0, NULL, width, height, 8, 0);
     ximg->data = malloc(ximg->bytes_per_line * height);
     memset(ximg->data, 0, ximg->bytes_per_line * height);

     return ximg;
}

int x11_init(int width, int height) {
     XWindowAttributes a;

     if ((d = XOpenDisplay(NULL)) == NULL)
          return -1;

     w = XCreateWindow(d, DefaultRootWindow(d), 0, 0, 100, 100, 0, CopyFromParent, InputOutput, CopyFromParent, 0, NULL);
     XMapWindow(d, w);
     gc = XCreateGC(d, w, 0, NULL);
     XGetWindowAttributes(d, w, &a);
     ximg = get_ximg(a.visual, width, height);
     initialized = true;
     
     return 0;
}

int x11_draw(AVFrame *frame) {
     int err;
     struct SwsContext *sws_ctx = NULL;
     int stride;

     if (initialized == false)
          err = x11_init(frame->width, frame->height);

     if (err)
          return -1;

     sws_ctx = sws_getCachedContext(sws_ctx,
                                    frame->width, frame->height, frame->format, 
                                    frame->width, frame->height, PIX_FMT_RGB32, 
                                    SWS_BICUBIC, NULL, NULL, 0);

     if (sws_ctx == NULL)
          return -1;

     stride = frame->width * ((32 + 7) / 8);
     sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, &ximg->data, &stride);
     XPutImage(d, w, gc, ximg, 0, 0, 0, 0, frame->width, frame->height);
     XFlush(d);

     return 0;
}
