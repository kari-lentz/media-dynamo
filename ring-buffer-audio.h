#ifndef RING_BUFFER_AUDIO
#define RING_BUFFER_AUDIO

#include "ring-buffer.h"
#include "my-audio-frame.h"
#include "messages.h"

//typedef ring_buffer<AME_AUDIO_FRAME> ring_buffer_audio_t;
//typedef ring_buffer< AME_AUDIO_FRAME > ring_buffer_audio_t;

class ring_buffer_audio_t:public ring_buffer<AME_AUDIO_FRAME>
{
public:
    int primed_message_;

ring_buffer_audio_t():ring_buffer<AME_AUDIO_FRAME>(),primed_message_(0)
    {
    };

    ring_buffer_audio_t(size_t frames_per_period, size_t periods, int primed_message):ring_buffer<AME_AUDIO_FRAME>(frames_per_period, periods), primed_message_(primed_message)
    {
    }
};


#endif
