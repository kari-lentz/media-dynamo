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

#include "video-file-context.h"

using namespace std;

void video_file_context::scale_frame2(AME_VIDEO_FRAME* frame)
{
    AVCodecContext* codec_context = get_codec_context();

    AVPicture pict = {{0}};

    frame->data[0] = frame->y_data;
    frame->data[1] = frame->u_data;
    frame->data[2] = frame->v_data;

    pict.data[0] = frame->data[0];
    pict.data[1] = frame->data[1];
    pict.data[2] = frame->data[2];

    pict.linesize[0] = overlay_->pitches[0];
    pict.linesize[1] = overlay_->pitches[2];
    pict.linesize[2] = overlay_->pitches[1];

    // FIXME use direct rendering
    av_picture_copy(&pict, (AVPicture *)frame_, PIX_FMT_YUV420P, overlay_->w, overlay_->h);

    frame->pts_ms = av_frame_get_best_effort_timestamp(frame_) * av_q2d(codec_context->time_base);

    frame->width = codec_context->width;
    frame->height = codec_context->height;
    frame->played_p = false;
    frame->skipped_p = false;
}

void video_file_context::scale_frame(AME_VIDEO_FRAME* frame)
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

    frame->pts_ms = av_frame_get_best_effort_timestamp(frame_) * av_q2d(codec_context->time_base);

    frame->width = codec_context->width;
    frame->height = codec_context->height;
    frame->played_p = false;
    frame->skipped_p = false;

    //caux << "decode frame:pts_ms:" << frame->pts_ms << ":"  << final_height << endl;
}

int video_file_context::decode_frames(AME_VIDEO_FRAME* frames, int size)
{
    int frame_ctr = 0;

    try
    {
        AVCodecContext* codec_context = get_codec_context();

        if( !codec_context )
        {
            stringstream ss;
            ss << "expected initialized codec context and its not there!!!";
            throw app_fault( ss.str().c_str() );
        }

        AVPacket pkt;

        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;

        while( frame_ctr < size )
        {
            int ret = 0;
            int got_frame = 0;

            /* read frames from the file */
            if( av_read_frame(format_context_, &pkt) >= 0 )
            {
                //caux << "read frame:dts:" << pkt.dts * av_q2d(codec_context->time_base) << ":pts:" << pkt.pts * av_q2d(codec_context->time_base) << endl;

                if (pkt.stream_index == stream_idx_)
                {
                    /* decode video frame */
                    ret = (*avcodec_decode_function_)(codec_context, frame_, &got_frame, &pkt);
                    if (ret < 0)
                    {
                        stringstream ss;
                        ss << "Error decoding video frame";
                        throw app_fault( ss.str().c_str() );
                    }

                    if( got_frame )
                    {
                        scale_frame(&frames[ frame_ctr ]);
                        ++frame_ctr;
                    }
                }
            }
            else
            {
                break;
            }

            av_free_packet(&pkt);
        }
    }
    catch( app_fault& e )
    {
        logger << e << endl;
        return -1;
    }

    return frame_ctr;
}

video_file_context::video_file_context(const char* mp4_file_path, ring_buffer_t* ring_buffer, SDL_Overlay* overlay):decode_context(mp4_file_path, AVMEDIA_TYPE_VIDEO, &avcodec_decode_video2),buffer_( ring_buffer ), overlay_(overlay), functor_(this, &video_file_context::decode_frames)
{
}

video_file_context::~video_file_context()
{
}

void video_file_context::operator()(int start_at)
{
    try
    {
        start_stream(start_at);

        int ret;

        do
        {
            ret = buffer_->write_period( &functor_ );
        } while(ret > 0);
    }
    catch(app_fault& e)
    {
        logger << e;
    }
}



