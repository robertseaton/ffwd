#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include <X11/Xlib.h>

#include <stdbool.h>
#include <pthread.h>

Display *d;
Window window;
GC gc;
XImage *ximg;
bool initialized = false;
int width; 
int height;
pthread_mutex_t x11mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t initial = PTHREAD_COND_INITIALIZER;

XWindowAttributes a;

void draw_subtitle(AVSubtitle *sub) {

}

XImage *get_ximg(Visual *v, int width, int height) {
     XImage *ximg;

     ximg = XCreateImage(d, v, 24, ZPixmap, 0, NULL, width, height, 8, 0);
     ximg->data = malloc(ximg->bytes_per_line * height);
     memset(ximg->data, 0, ximg->bytes_per_line * height);

     return ximg;
}

int x11_init(int w, int h) {

     XInitThreads();
     
     width = w;
     height = h;

     d = XOpenDisplay(NULL);
     window = XCreateWindow(d, DefaultRootWindow(d), 0, 0, width, height, 0, CopyFromParent, InputOutput, CopyFromParent, 0, NULL);
     XMapWindow(d, window);
     gc = XCreateGC(d, window, 0, NULL);
     XGetWindowAttributes(d, window, &a);
     ximg = get_ximg(a.visual, width, height);

     initialized = true;
     pthread_cond_signal(&initial);
     return 0;
}

int x11_draw(AVFrame *frame) {
     int err = 0;
     struct SwsContext *sws_ctx = NULL;
     int stride;
     static int w = 0;
     static int h = 0;

     if (initialized == false) 
          err = x11_init(frame->width, frame->height);
     

     if (err)
          return -1;

     pthread_mutex_lock(&x11mutex);
     if (w != width || h != height) {
          w = width;
          h = height;
          XDestroyImage(ximg);
          ximg = get_ximg(a.visual, w, h);
     }
     pthread_mutex_unlock(&x11mutex);

     sws_ctx = sws_getCachedContext(sws_ctx,
                                    frame->width, frame->height, frame->format, 
                                    w, h, PIX_FMT_RGB32, 
                                    SWS_BICUBIC, NULL, NULL, 0);

     if (sws_ctx == NULL)
          return -1;

     stride = w * ((32 + 7) / 8);
     sws_scale(sws_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0, frame->height, (uint8_t *const *)&ximg->data, &stride);

     XPutImage(d, window, gc, ximg, 0, 0, 0, 0, w, h);
     XFlush(d);

     return 0;
}
