#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <stdio.h>
#include <stdint.h>
#include "ring-buffer.h"
#include "app-fault.h"
#include "ring-buffer-video.h"
#include "logger.h"

using namespace std;

class display
{
private:

    ring_buffer_t* pbuffer_;
    specific_streamer<display, AME_VIDEO_FRAME> functor_;

    int call(AME_VIDEO_FRAME* pbuffer, int num_bytes);

public:

    static logger_t logger;

    display(ring_buffer_t* pbuffer);
    ~display();

    int operator()();
};

#endif
