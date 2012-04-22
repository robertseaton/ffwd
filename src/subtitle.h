#ifndef SUBTITLE
typedef struct SubtitleStruct *Subtitle;

Subtitle subtitle_create(uint8_t *data, int size, int pts, int duration);
#define SUBTITLE
#endif
