#ifndef RING_BUFFER_AUDIO
#define RING_BUFFER_AUDIO

#include "ring-buffer.h"

const int AUDIO_FRAME_SIZE = 1024;

template <int NUM_CHANNELS, int SAMPLES> struct ame_audio_frame_t
{
    short raw_data[SAMPLES * NUM_CHANNELS];
    short* data[1];
    int samples;
    int pts_ms;
    bool played_p;
    bool skipped_p;
};

typedef ame_audio_frame_t<2, AUDIO_FRAME_SIZE> AME_AUDIO_FRAME;

//typedef ring_buffer<AME_AUDIO_FRAME> ring_buffer_audio_t;
typedef ring_buffer<AME_AUDIO_FRAME> ring_buffer_audio_t;

#endif
