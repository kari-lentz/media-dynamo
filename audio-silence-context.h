#ifndef AUDIO_SILENCE_CONTEXT_H
#define AUDIO_SILENCE_CONTEXT_H

#include "decode-context.h"
#include "ring-buffer-audio.h"
#include "synch.h"

using namespace std;

typedef struct
{
    int channel;
    ring_buffer_audio_t* ring_buffer;
    ready_synch_t* buffer_ready;
    bool run_p;
    bool debug_p;
    int ret;
} env_audio_silence_context;

class audio_silence_context
{
private:
    ring_buffer_audio_t* buffer_;

    logger_t logger_;
    int min_frames_;

    specific_streamer<audio_silence_context, AME_AUDIO_FRAME> functor_;

    void write_frame();

public:

    audio_silence_context( ring_buffer_audio_t* ring_buffer);
    ~audio_silence_context();

    void operator()();
};

#endif
