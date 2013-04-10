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

#include "decode-context.h"
#include "logger.h"

using namespace std;

logger_t decode_context::logger( "VIDEO-PLAYER" );

void open_codec_context(int *stream_idx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret;
    AVStream *st;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *dec = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0)
    {
          stringstream ss;
          ss << "Could not find " << av_get_media_type_string(type) << " stream in input file";
          throw app_fault( ss.str().c_str() );
    }
    else
    {
        *stream_idx = ret;
        st = fmt_ctx->streams[*stream_idx];

        /* find decoder for the stream */
        dec_ctx = st->codec;
        dec = avcodec_find_decoder(dec_ctx->codec_id);
        if (!dec)
        {
            stringstream ss;
            ss << "Failed to find " << av_get_media_type_string(type) << "codec";
            throw app_fault( ss.str().c_str() );
        }

        if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0)
        {
            stringstream ss;
            ss << "Failed to open " << av_get_media_type_string(type) << "codec";
            throw app_fault( ss.str().c_str() );
        }
    }
}

void decode_context::scale_frame(AME_VIDEO_FRAME* frame)
{
    AVCodecContext* codec_context = get_codec_context();

    SwsContext*  sws_context = sws_getContext(codec_context->width, codec_context->height, codec_context->pix_fmt, codec_context->width, codec_context->height, PIX_FMT_YUV422P, SWS_BILINEAR, 0, 0, 0);

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

    for(int idx = 0; idx < 3; ++idx )
    {
        frame->linesize[ idx ] = frame_->linesize[ idx ];
    }

    sws_scale(sws_context, frame_->data, frame_->linesize, 0, codec_context->height, frame->data, frame->linesize);
    av_free( sws_context );

    frame->pts_ms = av_frame_get_best_effort_timestamp(frame_) * av_q2d(codec_context->time_base);

    frame_->width = codec_context->width;
    frame_->height = codec_context->height;
    frame->played_p = false;
    frame->skipped_p = false;
}

int decode_context::decode_frames(AME_VIDEO_FRAME* frames, int size)
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

        while( frame_ctr < size )
        {
            int ret = 0;
            int got_frame = 0;

            AVPacket pkt;

            av_init_packet(&pkt);
            pkt.data = NULL;
            pkt.size = 0;

            /* read frames from the file */
            if( av_read_frame(format_context_, &pkt) >= 0 )
            {
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

AVCodecContext* decode_context::get_codec_context()
{
    if( format_context_ && stream_idx_ >= 0 )
    {
        return format_context_->streams[ stream_idx_ ]->codec;
    }
    else
    {
        return NULL;
    }
}

void decode_context::log_callback (void* ptr, int level, const char* fmt, va_list ap)
{
    va_list ap_copy;

    va_copy( ap_copy, ap );

    const int LOG_BUFFER_SIZE = 1024;
    char buffer[LOG_BUFFER_SIZE];

    int ret = vsnprintf(buffer, LOG_BUFFER_SIZE ,fmt, ap_copy );

    for( int idx = 0; idx < ret; ++idx )
    {
        if( buffer[idx] == '\n' ) buffer[idx] = '|';
    }

    stringstream ss;
    ss.write( buffer, ret );

    logger << ss.str().c_str() << endl;

    va_end( ap_copy );
}

decode_context::decode_context(const char* mp4_file_path, ring_buffer_t* ring_buffer, AVMediaType type, avcodec_decode_function_t avcodec_decode_function):mp4_file_path_(mp4_file_path), type_(type), avcodec_decode_function_(avcodec_decode_function), stream_idx_(-1), buffer_( ring_buffer ), functor_(this, &decode_context::decode_frames), frame_(NULL),  write_pos_(0), max_pos_(0), remaining_stream_bytes_(0)
{
    av_log_set_callback( &decode_context::log_callback );
}

decode_context::~decode_context()
{
    caux << "closing file context" << endl;

    if( frame_ )
    {
        av_freep( frame_ );
        frame_ = NULL;
    }

    if( format_context_ )
    {
        AVCodecContext* c = NULL;

        /* Close each codec. */
        if (format_context_->nb_streams > 0)
        {
            c = format_context_->streams[0]->codec;
	    if( c ) avcodec_close( c );
        }

        /* free the entire format*/
        avformat_free_context(format_context_);
    }

    caux << "closing debug file if present" << endl;
}

void decode_context::call(int start_at)
{
    max_pos_ = 0;
    write_pos_ = 0;
    remaining_stream_bytes_ = 0;

    caux << "seek audio for:" << mp4_file_path_.c_str() << endl;

    /* open input file, and allocate format context */
    if (avformat_open_input(&format_context_, mp4_file_path_.c_str(), NULL, NULL) < 0)
    {
        stringstream ss;
        ss << "Could not open source file" << mp4_file_path_;
        throw app_fault( ss.str().c_str() );
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(format_context_, NULL) < 0)
    {
        stringstream ss;
        ss << "Could not find stream information";
        throw app_fault( ss.str().c_str() );
    }

    open_codec_context(&stream_idx_, format_context_, type_);

    if( !frame_ )
    {
        frame_ = avcodec_alloc_frame();
        if (!frame_)
        {
            stringstream ss;
            ss << "Could not allocate frame";
            throw app_fault( ss.str().c_str() );
        }
    }

    int ret;

    do
    {
        ret = buffer_->write_period( &functor_ );
    } while(ret > 0);
}

