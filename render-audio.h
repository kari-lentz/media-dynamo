#ifndef RENDER_AUDIO_H
#define RENDER_AUDIO_H

#include "render.h"
#include "ring-buffer-audio.h"

typedef struct
{
    ring_buffer_audio_t* ring_buffers;
    string alsa_dev;
    int num_channels;
    int periods_alsa;
    int wait_timeout;
    int frames_per_period;
    int periods_mpeg;
    ready_synch_t* buffer_ready;
    ready_synch_t* audio_ready;
    ready_synch_t* video_ready;
    bool debug_p;
    bool run_p;
    int ret;
} env_render_audio_context;

#include "alsa-engine.h"

#endif
