#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <stdio.h>
#include <stdint.h>
#include "ring-buffer.h"
#include "app-fault.h"
#include "ring-buffer-video.h"
#include "logger.h"
#include <SDL/SDL.h>

using namespace std;

typedef struct
{
    ring_buffer_video_t* ring_buffer;
    SDL_Overlay* overlay;
    bool debug_p;
    int ret;
} env_display_context;

class display
{
private:

    Uint32 begin_tick_ms_;
    ring_buffer_video_t* pbuffer_;
    specific_streamer<display, AME_VIDEO_FRAME> functor_;
    SDL_Surface* screen_;
    SDL_Overlay* overlay_;
    bool error_p_;

    int display_frame(AME_VIDEO_FRAME* pframes, int num_frames);
    int call(AME_VIDEO_FRAME* pbuffer, int num_bytes);

public:

    static logger_t logger;

    display(ring_buffer_video_t* pbuffer, SDL_Overlay* overlay);
    ~display();

    int operator()();
};

#endif
