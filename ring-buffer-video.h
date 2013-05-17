#ifndef RING_BUFFER_VIDEO
#define RING_BUFFER_VIDEO

#include "ring-buffer.h"

typedef struct
{
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} ARGB;

const int MAX_SCREEN_WIDTH = 1920;
const int MAX_SCREEN_HEIGHT = 1080;

//const int MAX_SCREEN_WIDTH = 1536;
//const int MAX_SCREEN_HEIGHT= 864;

typedef struct
{
    int width;
    int height;
    uint8_t* data[4];
    uint8_t raw_data[ 4 * MAX_SCREEN_WIDTH * MAX_SCREEN_HEIGHT ];
    int linesize[ 4 ];
} AME_MIXER_FRAME;

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

typedef ring_buffer<AME_VIDEO_FRAME> ring_buffer_video_t;

#endif
