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

#include "audio-decode-context.h"
#include "sdl-holder.h"

using namespace std;

template <> logger_t decode_context<AME_AUDIO_FRAME>::logger("AUDIO-PLAYER");

void audio_decode_context::write_frame(AME_AUDIO_FRAME* frame)
{
    struct SwrContext* ctx;

    ctx = swr_alloc_set_opts ( NULL,
                               av_get_default_channel_layout( 2 ),
                               AV_SAMPLE_FMT_S16,
                               44100,
                               frame_->channel_layout,
                               (AVSampleFormat) frame_->format,
                               frame_->sample_rate,
                               0,
                               NULL);

    if( ctx )
    {
        frame->data[0] = &frame->raw_data[0];

        swr_convert( ctx,
                     (uint8_t**) frame->data,
                     4,
                     (const uint8_t**) frame_->extended_data,
                     frame_->nb_samples);

        swr_free(&ctx);
    }
    else
    {
        logger << "no sample context allocated" << endl;
    }


    memcpy( frame->data, frame_->data, 4 * sizeof( short ) );

    int best_pts = av_frame_get_best_effort_timestamp(frame_) * av_q2d(format_context_->streams[ stream_idx_ ]->time_base) * 1000;
    //int pkt_pts = frame_->pkt_dts * av_q2d(format_context_->streams[ stream_idx_ ]->time_base) * 1000;
    frame->pts_ms = best_pts;
    frame->played_p = false;
    frame->skipped_p = false;

    //caux << "decode frame:pts_ms:" << best_pts << ":"  << pkt_pts << endl;
}

audio_decode_context::audio_decode_context(const char* mp4_file_path, ring_buffer_audio_t* ring_buffer):decode_context(mp4_file_path, AVMEDIA_TYPE_AUDIO, &avcodec_decode_audio4),buffer_( ring_buffer ), functor_(this, &decode_context::decode_frames)
{
}

audio_decode_context::~audio_decode_context()
{
}

void audio_decode_context::operator()(int start_at)
{
    try
    {
        start_stream(start_at);

        do
        {
            buffer_->write_period( &functor_ );

        } while(!buffer_->is_done());
    }
    catch(app_fault& e)
    {
        logger << e;
    }
}



