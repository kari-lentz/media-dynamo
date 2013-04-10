#ifndef RING_BUFFER_VIDEO
#define RING_BUFFER_VIDEO

#include "ring-buffer.h"

typedef struct
{
    int width;
    int height;
    int pts_ms;
    bool played_p;
    bool skipped_p;
    int linesize[ 3 ];
    uint8_t* data[ 3 ];
    uint8_t y_data[ 1920 * 1080 ];
    uint8_t u_data[ 1920 * 1080 / 2 ];
    uint8_t v_data[ 1920 * 1080 / 2 ];
} AME_VIDEO_FRAME;

typedef ring_buffer<AME_VIDEO_FRAME> ring_buffer_t;

#endif
