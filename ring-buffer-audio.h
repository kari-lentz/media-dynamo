#ifndef RING_BUFFER_AUDIO
#define RING_BUFFER_AUDIO

#include "ring-buffer.h"

typedef struct
{
    short data[4];
    int pts_ms;
    bool played_p;
    bool skipped_p;
} AME_AUDIO_FRAME;

//typedef ring_buffer<AME_AUDIO_FRAME> ring_buffer_audio_t;
typedef ring_buffer<AME_AUDIO_FRAME> ring_buffer_audio_t;

#endif
