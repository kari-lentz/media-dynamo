#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include "display.h"

using namespace std;

logger_t display::logger("display");

const char* COUNTS_MANAGER = "COUNTS-MANAGER";

int display::call(unsigned char* pbuffer, int num_bytes)
{
    int bytes = 0;
    return bytes;
}

display::display(ring_buffer_t* pbuffer):pbuffer_(pbuffer), functor_(this, &display::call)
{
}

display::~display()
{
}

int display::operator()()
{
    int ret;

    do
    {
        ret = pbuffer_->read_avail( &functor_ );
    }  while( ret > 0);

    return ret;
}
