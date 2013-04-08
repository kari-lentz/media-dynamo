#ifndef ENCODE_CONTEXT_H
#define ENCODE_CONTEXT_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/audioconvert.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
}

#include <stdio.h>
#include <string>
#include <fstream>
#include <iostream>
#include <exception>
#include "app-fault.h"
#include "vwriter.h"
#include "ring-buffer-uint8.h"
#include "logger.h"

using namespace std;

#define OUT_BUFFER_SIZE 16384

typedef vwriter<uint8_t> vwriter_t;

typedef void* (*encode_context_thread_t)(void*);
typedef int (*avcodec_decode_function_t) (AVCodecContext *avctx, AVFrame *picture, int *got_picture_ptr, const AVPacket *avpkt);

typedef struct
{
    const char* mp4_file_path;
    int start_at;
    ring_buffer_t* ring_buffer;
    bool debug_p;
    int ret;
} env_file_context;

class decode_context
{
private:

    static void log_callback (void* ptr, int level, const char* fmt, va_list ap);

protected:

    string mp4_file_path_;
    AVMediaType type_;
    avcodec_decode_function_t avcodec_decode_function_;

    int stream_idx_;
    vwriter_t vwriter_;
    AVFrame* frame_;
    size_t write_pos_;
    size_t max_pos_;
    int remaining_stream_bytes_;
    bool eof_p_;
    bool error_p_;

    AVFormatContext *format_context_;

    void write_ring_buffer(uint8_t* buffer, size_t len);

    void decode_packet(AVFrame* frame, int *got_frame, AVPacket& pkt);
    void call(int start_at);

public:

    static logger_t logger;

    AVCodecContext* get_codec_context();

    // handle various input formats
    //
    decode_context(const char* mp4_file_path, ring_buffer_t* ring_buffer, AVMediaType type, avcodec_decode_function_t avcodec_decode_function);
    virtual ~decode_context();

    void operator()(int start_at = 0);
};

#endif
