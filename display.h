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

class display
{
private:

    int media_ms_;
    ring_buffer_t* pbuffer_;
    specific_streamer<display, AME_VIDEO_FRAME> functor_;
    SDL_Surface* screen_;
    SDL_Overlay* overlay_;

    int display_frame(AME_VIDEO_FRAME* pframes, int num_frames);
    int call(AME_VIDEO_FRAME* pbuffer, int num_bytes);

public:

    static logger_t logger;

    display(ring_buffer_t* pbuffer, SDL_Overlay* overlay);
    ~display();

    int operator()();
};

#endif
