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

#include "messages.h"
#include "audio-decode-context.h"
#include "sdl-holder.h"

using namespace std;

template <> logger_t decode_context<AME_AUDIO_FRAME>::logger("AUDIO-PLAYER");

void audio_decode_context::buffer_primed()
{
    post_message( buffer_->primed_message_ );
}

int audio_decode_context::write_frame_to_buffer(AVFrame* frame_in, AME_AUDIO_FRAME& frame_out, int samples, int start_at)
{
    int best_pts = av_frame_get_best_effort_timestamp(frame_) * av_q2d(format_context_->streams[ stream_idx_ ]->time_base) * 1000;
    //int pkt_pts = frame_->pkt_dts * av_q2d(format_context_->streams[ stream_idx_ ]->time_base) * 1000;
    frame_out.pts_ms = best_pts;
    frame_out.start_at = start_at;
    frame_out.played_p = false;
    frame_out.skipped_p = false;

    frame_out.samples = samples;
    frame_out.data[0] = &frame_out.raw_data[0][0];

    //caux_video << "AUDIO BEGIN DECODDE FRAME at:" << best_pts << endl;
    int ret = vwriter<AME_AUDIO_FRAME>(buffer_, false)( &frame_out, 1);
    //caux_video << "AUDIO END DECODDE FRAME at:" << best_pts << ":" << ret << endl;

    if( ret <= 0 ) throw decode_done_t();

    //logger << "WROTE TO ZONE 1 AUDIO BUFFER:" << frame_out.samples << endl;

    return ret;
}

void audio_decode_context::write_frame(AVFrame* frame_in, int start_at)
{
    //copy from ffmpeg native format to the 44100 format for ALSA
    //
    STEREO_AUDIO_FRAME frame_stereo;

    struct SwrContext* ctx;

    ctx = swr_alloc_set_opts ( NULL,
                               av_get_default_channel_layout( 2 ),
                               AV_SAMPLE_FMT_S16,
                               RATE,
                               frame_in->channel_layout,
                               (AVSampleFormat) frame_in->format,
                               frame_in->sample_rate,
                               0,
                               NULL);

    if( ctx )
    {
        if (swr_set_compensation(ctx, (AUDIO_PACKET_SIZE - frame_in->nb_samples) * RATE / frame_in->sample_rate,
                                 AUDIO_PACKET_SIZE * RATE / frame_in->sample_rate) < 0)
        {
            throw app_fault( "swr_set_compensation() failed" );
        }

        if( swr_init( ctx ) < 0 )
        {
            throw app_fault("swr_init failed");
        }

        frame_stereo.data[0] = &frame_stereo.raw_data[0][0];

        int ret_convert = 0;
        bool buffering_p = false;

        int remaining = AUDIO_PACKET_SIZE * RATE / frame_in->sample_rate;

        while(remaining > 0)
        {
            uint8_t** in;
            int nb_samples;

            if( buffering_p )
            {
                in = NULL;
                nb_samples = 0;
            }
            else
            {
                in = frame_in->extended_data;
                nb_samples = frame_in->nb_samples;
            }

            ret_convert = swr_convert( ctx,
                                       (uint8_t**) frame_stereo.data,
                                       AUDIO_PACKET_SIZE,
                                       (const uint8_t**) in,
                                       nb_samples);

            remaining -= ret_convert;

            //so null paramters go to swr convert the next time
            //
            buffering_p = true;

            //now down mix
            //
            for(int sample = 0; sample < ret_convert; ++sample)
            {
                frame_out_.raw_data[frame_out_idx_][0] = (frame_stereo.raw_data[sample][0] + frame_stereo.raw_data[sample][1]) / 2;

                ++frame_out_idx_;

                if(frame_out_idx_ >= AUDIO_PACKET_SIZE)
                {
                    write_frame_to_buffer(frame_in, frame_out_, AUDIO_PACKET_SIZE, start_at);
                    frame_out_idx_ = 0;
                }
            }

            if( AUDIO_PACKET_SIZE == ret_convert )
            {
                logger << "swr_convert returned " << ret_convert << " audio buffer too small" << endl;
            }

        }

        swr_free(&ctx);
    }
    else
    {
        logger << "no sample context allocated" << endl;
    }
}

void audio_decode_context::flush_frame(AVFrame* frame_in, int start_at)
{
    if( frame_out_idx_ > 0 )
    {
        write_frame_to_buffer(frame_in, frame_out_, frame_out_idx_, start_at);
        frame_out_idx_ = 0;
    }
}

audio_decode_context::audio_decode_context(const char* mp4_file_path, ring_buffer_audio_t* ring_buffer):decode_context(mp4_file_path, AVMEDIA_TYPE_AUDIO, &avcodec_decode_audio4, ring_buffer->get_frames_per_period() * (ring_buffer->get_periods() - 1)), buffer_( ring_buffer ), frame_out_idx_(0)
{
}

audio_decode_context::~audio_decode_context()
{
}

void audio_decode_context::operator()(int start_at)
{
    try
    {
        iter_frames(start_at);
    }
    catch(decode_done_t& e)
    {
        vwriter<AME_AUDIO_FRAME>(buffer_, false).close();
    }
    catch(app_fault& e)
    {
        logger << e;
        vwriter<AME_AUDIO_FRAME>(buffer_, false).error();
    }
}



