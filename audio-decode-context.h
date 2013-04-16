#ifndef AUDIO_DECODE_CONTEXT_H
#define AUDIO_DECODE_CONTEXT_H

#include "decode-context.h"
#include "ring-buffer-audio.h"

using namespace std;

typedef struct
{
    const char* mp4_file_path;
    int start_at;
    ring_buffer_audio_t* ring_buffer;
    bool debug_p;
    int ret;
} env_audio_decode_context;

class audio_decode_context:public decode_context<AME_AUDIO_FRAME>
{
private:
    ring_buffer_audio_t* buffer_;

    specific_streamer<audio_decode_context, AME_AUDIO_FRAME> functor_;

    AVFormatContext* oc_;
    FILE* outfile_;

    void write_frame(AME_AUDIO_FRAME* frame);
    int decode_frames(AME_AUDIO_FRAME* frames, int size);

public:

    audio_decode_context(const char* mp4_file_path, ring_buffer_audio_t* ring_buffer);
    ~audio_decode_context();

    void operator()(int start_at = 0);
};

#endif
