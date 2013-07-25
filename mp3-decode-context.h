#ifndef AUDIO_SILENCE_CONTEXT_H
#define AUDIO_SILENCE_CONTEXT_H

#include <mysql++/mysql++.h>
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
    mysqlpp::Connection* conn;
} env_mp3_decode_context;

class mp3_decode_context
{
private:
    ring_buffer_audio_t* buffer_;

    logger_t logger_;
    int min_frames_;

    specific_streamer<mp3_decode_context, AME_AUDIO_FRAME> functor_;

    void write_frame(int start_at);

public:

    mp3_decode_context( ring_buffer_audio_t* ring_buffer);
    ~mp3_decode_context();

    void operator()();
};

#endif
