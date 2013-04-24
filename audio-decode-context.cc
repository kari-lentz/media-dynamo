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

void audio_decode_context::write_frame(AVFrame* frame_in)
{
    //copy from ffmpeg native format to the 44100 format for ALSA
    //
    STEREO_AUDIO_FRAME frame_stereo;
    AME_AUDIO_FRAME frame_out;

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
        frame_stereo.data[0] = &frame_stereo.raw_data[0][0];

        int ret = 0;
        do
        {

            ret = swr_convert( ctx,
                               (uint8_t**) frame_stereo.data,
                               AUDIO_PACKET_SIZE,
                               (const uint8_t**) frame_in->extended_data,
                               frame_in->nb_samples);

            //now down mix
            for(int sample = 0; sample < ret; ++sample)
            {
                frame_out.raw_data[sample][0] = frame_stereo.raw_data[sample][0] + frame_stereo.raw_data[sample][1];
            }

            frame_out.samples = ret;

            int best_pts = av_frame_get_best_effort_timestamp(frame_in) * av_q2d(format_context_->streams[ stream_idx_ ]->time_base) * 1000;
            //int pkt_pts = frame_in->pkt_dts * av_q2d(format_context_->streams[ stream_idx_ ]->time_base) * 1000;
            frame_out.pts_ms = best_pts;

            frame_out.played_p = false;
            frame_out.skipped_p = false;

            int ret = vwriter<AME_AUDIO_FRAME>(buffer_)(&frame_out, 1);
            if( ret ) throw decode_done_t();

        } while (ret == AUDIO_PACKET_SIZE);

        swr_free(&ctx);
    }
    else
    {
        logger << "no sample context allocated" << endl;
    }

    int best_pts = av_frame_get_best_effort_timestamp(frame_) * av_q2d(format_context_->streams[ stream_idx_ ]->time_base) * 1000;
    //int pkt_pts = frame_->pkt_dts * av_q2d(format_context_->streams[ stream_idx_ ]->time_base) * 1000;
    frame_out.pts_ms = best_pts;
    frame_out.played_p = false;
    frame_out.skipped_p = false;

    int ret = vwriter<AME_AUDIO_FRAME>(buffer_, false)( &frame_out, 1);
    if( ret <= 0 ) throw decode_done_t();
}

audio_decode_context::audio_decode_context(const char* mp4_file_path, ring_buffer_audio_t* ring_buffer, int channel):decode_context(mp4_file_path, AVMEDIA_TYPE_AUDIO, &avcodec_decode_audio4),buffer_( &ring_buffer[channel] )
{
}

audio_decode_context::~audio_decode_context()
{
}

void audio_decode_context::operator()(int start_at)
{
    iter_frames(start_at);
}



