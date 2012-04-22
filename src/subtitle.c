typedef struct sbtl {
     uint8_t *data;
     int sz;
     int pts;
     int duration;
} SubtitleStruct;


Subtitle subtitle_create(uint8_t *data, int sz, int pts, int duration) {
     SubtitleStruct *sb = malloc(sizeof(SubtitleStruct));
     memcpy(sb->data, data, sz);
     sb->sz = sz;
     sb->pts = pts;
     sb->duration = duration;

     return sb;
}

Subtitle subtitle_get_pts(Subtitle _s) {
     SubtitleStruct *s = _s;

     return s->pts;
}

Subtitle subtitle_get_duration(Subtitle _s) {
     SubtitleStruct *s = _s;

     return s->duration;
}

