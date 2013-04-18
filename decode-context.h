#ifndef DECODE_CONTEXT_H
#define DECODE_CONTEXT_H

#include "ffmpeg-headers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include <iostream>
#include <exception>
#include <SDL/SDL.h>
#include "app-fault.h"
#include "logger.h"
#include "decode-context.h"
#include "logger.h"

using namespace std;

typedef void* (*encode_context_thread_t)(void*);
typedef int (*avcodec_decode_function_t) (AVCodecContext *avctx, AVFrame *picture, int *got_picture_ptr, const AVPacket *avpkt);

template <typename T> class decode_context
{
private:

    static void open_codec_context(int *stream_idx, AVFormatContext *fmt_ctx, enum AVMediaType type)
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
            caux << "found video stream id:" << *stream_idx << endl;
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

            caux << "opened decoder" << endl;
        }
    }

    static void log_callback (void* ptr, int level, const char* fmt, va_list ap)
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

        decode_context<T>::logger << ss.str().c_str() << endl;

        va_end( ap_copy );
    }

protected:

    string mp4_file_path_;
    AVMediaType type_;
    avcodec_decode_function_t avcodec_decode_function_;

    int stream_idx_;

    AVFrame* frame_;
    size_t write_pos_;
    size_t max_pos_;
    int remaining_stream_bytes_;

    AVFormatContext *format_context_;
    AVPacket packet_;
    AVPacket packet_temp_;
    bool error_p_;

    void start_stream(int start_at = 0)
    {
        max_pos_ = 0;
        write_pos_ = 0;
        remaining_stream_bytes_ = 0;
        error_p_ = false;

        caux << "seek audio for:" << mp4_file_path_.c_str() << endl;

        /* open input file, and allocate format context */
        if (avformat_open_input(&format_context_, mp4_file_path_.c_str(), NULL, NULL) < 0)
        {
            stringstream ss;
            ss << "Could not open source file" << mp4_file_path_;
            throw app_fault( ss.str().c_str() );
        }

        caux << "opened input file:" << mp4_file_path_ << endl;

        /* retrieve stream information */
        if (avformat_find_stream_info(format_context_, NULL) < 0)
        {
            stringstream ss;
            ss << "Could not find stream information";
            throw app_fault( ss.str().c_str() );
        }

        open_codec_context(&stream_idx_, format_context_, type_);

        caux << "opened codec context" << endl;

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
    }

    virtual void write_frame(T* frame)=0;

public:

    int decode_frames(T* frames, int size)
    {
        int frame_ctr = 0;

        if( error_p_ ) return -1;

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

                if( packet_temp_.size == 0 )
                {
                    /* read frames from the file */
                    if( av_read_frame(format_context_, &packet_) >= 0 )
                    {
                        packet_temp_.size = packet_.size;
                        packet_temp_.data = packet_.data;
                        packet_temp_.stream_index = packet_.stream_index;
                        packet_temp_.pts = packet_.pts;
                        packet_temp_.dts = packet_.dts;
                    }
                    else
                    {
                        return 0;
                    }
                }

                if (packet_temp_.stream_index == stream_idx_)
                {
                    //caux << "raw read frame:size:" << packet_.size  << ":pts:" << packet_.pts * av_q2d(codec_context->time_base) << endl;

                    /* decode frame */
                    ret = (*avcodec_decode_function_)(codec_context, frame_, &got_frame, &packet_temp_);
                    if (ret < 0)
                    {
                        stringstream ss;
                        ss << "Error decoding video frame";
                        throw app_fault( ss.str().c_str() );
                    }

                    if( got_frame )
                    {
                        write_frame(&frames[ frame_ctr ]);
                        ++frame_ctr;
                    }

                    packet_temp_.size -= ret;
                    packet_temp_.data += ret;

                    if( packet_temp_.size <= 0 )
                    {
                        av_free_packet( &packet_ );
                        av_init_packet( &packet_ );
                        packet_.size = 0;
                        packet_.data = NULL;
                        packet_temp_.size = 0;
                        packet_temp_.data = NULL;
                    }
                }
                else
                {
                    av_free_packet( &packet_ );
                    av_init_packet( &packet_ );
                    packet_.size = 0;
                    packet_.data = NULL;
                    packet_temp_.size = 0;
                    packet_temp_.data = NULL;
                }
            }
        }
        catch( app_fault& e )
        {
            logger << e << endl;
            return -1;
        }

        return frame_ctr;
    }

    static logger_t logger;

    AVCodecContext* get_codec_context()
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

    // handle various input formats
    //
decode_context(const char* mp4_file_path, AVMediaType type, avcodec_decode_function_t avcodec_decode_function):mp4_file_path_(mp4_file_path), type_(type), avcodec_decode_function_(avcodec_decode_function), error_p_(false)
    {
        av_log_set_callback( &decode_context::log_callback );

        av_init_packet( &packet_ );
        packet_.data = NULL;
        packet_.size = 0;

        packet_temp_.data = NULL;
        packet_temp_.size = 0;
        av_init_packet( &packet_temp_ );
    }

    virtual ~decode_context()
    {
        av_free_packet( &packet_ );
        av_free_packet( &packet_temp_ );
        caux << "closing file context" << endl;

        if( frame_ )
        {
            av_free( frame_ );
            frame_ = NULL;
        }

        caux << "freed frame" << endl;

        if( format_context_ )
        {
            AVCodecContext* c = NULL;

            /* Close each codec. */
            if (format_context_->nb_streams > 0)
            {
                c = format_context_->streams[0]->codec;
                if( c ) avcodec_close( c );
            }

            caux << "closed codec" << endl;

            /* free the entire format*/
            avformat_free_context(format_context_);
        }

        caux << "decoder destroyed" << endl;
    }

};

#endif
