#ifndef RING_BUFFER_VIDEO
#define RING_BUFFER_VIDEO

#include "ring-buffer.h"

typedef struct
{
    int pts_ms;
    uint8_t data[ 2 * 1920 * 1080 ];
    int linesize[ 3 ];
} AME_VIDEO_FRAME;

typedef ring_buffer<AME_VIDEO_FRAME> ring_buffer_t;

#endif
