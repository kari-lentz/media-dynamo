#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include <stdint.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <string>
#include <sstream>

#include "file-context.h"

using namespace std;

file_context::file_context(const char* mp4_file_path, ring_buffer_t* ring_buffer):decode_context(mp4_file_path, ring_buffer, AVMEDIA_TYPE_VIDEO, &avcodec_decode_video2)
{
}

file_context::~file_context()
{
}

void file_context::operator()(int start_at)
{
    call(start_at);
}


