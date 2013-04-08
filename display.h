#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <stdio.h>
#include <stdint.h>
#include "ring-buffer.h"
#include "app-fault.h"
#include "ring-buffer-uint8.h"
#include "logger.h"

using namespace std;

class display
{
private:

    ring_buffer_t* pbuffer_;
    specific_streamer<display, uint8_t> functor_;

    int call(uint8_t* pbuffer, int num_bytes);

public:

    static logger_t logger;

    display(ring_buffer_t* pbuffer);
    ~display();

    int operator()();
};

#endif
