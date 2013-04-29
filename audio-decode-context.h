#ifndef AUDIO_DECODE_CONTEXT_H
#define AUDIO_DECODE_CONTEXT_H

#include "decode-context.h"
#include "ring-buffer-audio.h"
#include "synch.h"

using namespace std;

typedef struct
{
    const char* mp4_file_path;
    int start_at;
    ring_buffer_audio_t* ring_buffer;
    ready_synch_t* buffer_ready;
    bool run_p;
    bool debug_p;
    int ret;
} env_audio_decode_context;

class audio_decode_context:public decode_context<AME_AUDIO_FRAME>
{
private:
    ring_buffer_audio_t* buffer_;

    specific_streamer<audio_decode_context, AME_AUDIO_FRAME> functor_;

    AVFormatContext* oc_;

    void write_frame(AVFrame* frame_in);

public:

    audio_decode_context(const char* mp4_file_path, ring_buffer_audio_t* ring_buffer, int channel);
    ~audio_decode_context();

    void operator()(int start_at = 0);
};

#endif
