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

void video_decode_context::write_frame(AME_VIDEO_FRAME* frame)
{
    AVCodecContext* codec_context = get_codec_context();

    SwsContext*  sws_context = sws_getContext(frame_->width, frame_->height, (PixelFormat) frame_->format, overlay_->pitches[0], overlay_->h, PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);

    if( !sws_context )
    {
        stringstream ss;
        ss << "failed to get scale context";
        throw app_fault( ss.str().c_str() );
    }

    //Scale the raw data/convert it to our video buffer...
    //
    frame->data[0] = frame->y_data;
    frame->data[1] = frame->u_data;
    frame->data[2] = frame->v_data;

    frame->linesize[ 0 ] = overlay_->pitches[ 0 ];
    frame->linesize[ 2 ] = overlay_->pitches[ 1 ];
    frame->linesize[ 1 ] = overlay_->pitches[ 2 ];

    sws_scale(sws_context, frame_->data, frame_->linesize, 0, frame_->height, frame->data, frame->linesize);
    av_free( sws_context );

    int best_pts = av_frame_get_best_effort_timestamp(frame_) * av_q2d(format_context_->streams[ stream_idx_ ]->time_base) * 1000;
    //int pkt_pts = frame_->pkt_dts * av_q2d(format_context_->streams[ stream_idx_ ]->time_base) * 1000;
    frame->pts_ms = best_pts;

    frame->width = codec_context->width;
    frame->height = codec_context->height;
    frame->played_p = false;
    frame->skipped_p = false;

    //caux << "decode frame:pts_ms:" << best_pts << ":"  << pkt_pts << endl;
}

video_decode_context::video_decode_context(const char* mp4_file_path, ring_buffer_video_t* ring_buffer, SDL_Overlay* overlay):decode_context(mp4_file_path, AVMEDIA_TYPE_VIDEO, &avcodec_decode_video2),buffer_( ring_buffer ), overlay_(overlay), functor_(this, &decode_context::decode_frames)
{
}

video_decode_context::~video_decode_context()
{
}

void video_decode_context::operator()(int start_at)
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



