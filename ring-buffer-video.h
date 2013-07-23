#ifndef RING_BUFFER_VIDEO
#define RING_BUFFER_VIDEO

#include <SDL/SDL.h>
#include "ring-buffer.h"

typedef struct
{
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} ARGB;

//const int MAX_SCREEN_WIDTH = 1024;
//const int MAX_SCREEN_HEIGHT = 768;

//const int MAX_SCREEN_WIDTH = 1536;
//const int MAX_SCREEN_HEIGHT= 864;

const int MAX_SCREEN_WIDTH = 1920;
const int MAX_SCREEN_HEIGHT = 1080;

typedef struct
{
    int width;
    int height;
    int pts_ms;
    int start_at;
    bool played_p;
    bool skipped_p;
    uint8_t* data[4];
    uint8_t raw_data[ 4 * MAX_SCREEN_WIDTH * MAX_SCREEN_HEIGHT ];
    int linesize[ 4 ];
} AME_VIDEO_FRAME;

class ring_buffer_video_t:public ring_buffer<AME_VIDEO_FRAME>
{
public:
    int width_;
    int height_;

    ring_buffer_video_t(size_t frames_per_period, size_t periods, int width, int height):ring_buffer<AME_VIDEO_FRAME>(frames_per_period, periods), width_(width), height_(height)
    {
    }
};

#endif
