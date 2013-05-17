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

#include "video-decode-context.h"
#include "sdl-holder.h"

using namespace std;

template <> logger_t decode_context<AME_VIDEO_FRAME>::logger("VIDEO-PLAYER");

AME_MIXER_FRAME frame_mix;
AME_VIDEO_FRAME frame_out;

void video_decode_context::buffer_primed()
{
    video_primed_->signal(true);
}

void video_decode_context::write_frame(AVFrame* frame_in)
{
    AVCodecContext* codec_context = get_codec_context();

    SwsContext*  sws_context = sws_getContext(frame_in->width, frame_in->height, (PixelFormat) frame_in->format, overlay_->pitches[0], overlay_->h, PIX_FMT_RGB24, SWS_BICUBIC, 0, 0, 0);

    //PIX_FMT_YUV420P

    if( !sws_context )
    {
        stringstream ss;
        ss << "failed to get scale context";
        throw app_fault( ss.str().c_str() );
    }

    //Scale the raw data/convert it to our video buffer...
    //
    frame_mix.data[0] = frame_mix.raw_data;
    frame_mix.data[1] = NULL;
    frame_mix.data[2] = NULL;
    frame_mix.data[3] = NULL;

    frame_mix.linesize[ 0 ] = 3 * overlay_->pitches[0];
    frame_mix.linesize[ 1 ] = 0;
    frame_mix.linesize[ 2 ] = 0;
    frame_mix.linesize[ 3 ] = 0;

    sws_scale(sws_context, frame_in->data, frame_in->linesize, 0, frame_in->height, frame_mix.data, frame_mix.linesize);

    av_free( sws_context );

    sws_context = sws_getContext(overlay_->pitches[0], overlay_->h, PIX_FMT_RGB24, overlay_->pitches[0], overlay_->h, PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);

    if( !sws_context )
    {
        stringstream ss;
        ss << "failed to get scale context";
        throw app_fault( ss.str().c_str() );
    }

    //Scale the raw data/convert it to our video buffer...
    //
    frame_out.data[0] = frame_out.y_data;
    frame_out.data[1] = frame_out.u_data;
    frame_out.data[2] = frame_out.v_data;

    frame_out.linesize[ 0 ] = overlay_->pitches[ 0 ];
    frame_out.linesize[ 2 ] = overlay_->pitches[ 1 ];
    frame_out.linesize[ 1 ] = overlay_->pitches[ 2 ];

    sws_scale(sws_context, frame_mix.data, frame_mix.linesize, 0, overlay_->h, frame_out.data, frame_out.linesize);

    av_free( sws_context );

    int best_pts = av_frame_get_best_effort_timestamp(frame_in) * av_q2d(format_context_->streams[ stream_idx_ ]->time_base) * 1000;
    //int pkt_pts = frame_in->pkt_dts * av_q2d(format_context_->streams[ stream_idx_ ]->time_base) * 1000;
    frame_out.pts_ms = best_pts;

    frame_out.width = codec_context->width;
    frame_out.height = codec_context->height;
    frame_out.played_p = false;
    frame_out.skipped_p = false;

    int ret = vwriter<AME_VIDEO_FRAME>(buffer_, false)( &frame_out, 1);

    if( ret <= 0 ) throw decode_done_t();
}

video_decode_context::video_decode_context(const char* mp4_file_path, ring_buffer_video_t* ring_buffer, SDL_Overlay* overlay, ready_synch_t* video_primed):decode_context(mp4_file_path, AVMEDIA_TYPE_VIDEO, &avcodec_decode_video2, ring_buffer->get_frames_per_period() * (ring_buffer->get_periods() - 1) - ring_buffer->get_available_samples()),buffer_( ring_buffer ), overlay_(overlay), video_primed_(video_primed)
{
}

video_decode_context::~video_decode_context()
{
}

void video_decode_context::operator()(int start_at)
{
    try
    {
        iter_frames(start_at);
    }
    catch(decode_done_t& e)
    {
        vwriter<AME_VIDEO_FRAME>(buffer_, false).close();
    }
    catch(app_fault& e)
    {
        logger << e;
        vwriter<AME_VIDEO_FRAME>(buffer_, false).error();
    }
}



