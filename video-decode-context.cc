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

void video_decode_context::write_frame(AVFrame* frame_in)
{
    AME_VIDEO_FRAME frame_out;

    AVCodecContext* codec_context = get_codec_context();

    SwsContext*  sws_context = sws_getContext(frame_in->width, frame_in->height, (PixelFormat) frame_in->format, overlay_->pitches[0], overlay_->h, PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);

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

    sws_scale(sws_context, frame_in->data, frame_in->linesize, 0, frame_in->height, frame_out.data, frame_out.linesize);
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

    //caux << "decode frame_out:pts_ms:" << best_pts << ":"  << pkt_pts << endl;
}

video_decode_context::video_decode_context(const char* mp4_file_path, ring_buffer_video_t* ring_buffer, SDL_Overlay* overlay):decode_context(mp4_file_path, AVMEDIA_TYPE_VIDEO, &avcodec_decode_video2),buffer_( ring_buffer ), overlay_(overlay)
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



