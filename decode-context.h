#ifndef ENCODE_CONTEXT_H
#define ENCODE_CONTEXT_H

#include "ffmpeg-headers.h"

#include <stdio.h>
#include <string>
#include <fstream>
#include <iostream>
#include <exception>
#include <SDL/SDL.h>
#include "app-fault.h"
#include "ring-buffer-video.h"
#include "logger.h"

using namespace std;

typedef void* (*encode_context_thread_t)(void*);
typedef int (*avcodec_decode_function_t) (AVCodecContext *avctx, AVFrame *picture, int *got_picture_ptr, const AVPacket *avpkt);

typedef struct
{
    const char* mp4_file_path;
    int start_at;
    ring_buffer_t* ring_buffer;
    SDL_Overlay* overlay;
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

    AVFrame* frame_;
    size_t write_pos_;
    size_t max_pos_;
    int remaining_stream_bytes_;

    AVFormatContext *format_context_;

    void start_stream(int start_at = 0);

public:

    static logger_t logger;

    AVCodecContext* get_codec_context();

    // handle various input formats
    //
    decode_context(const char* mp4_file_path, AVMediaType type, avcodec_decode_function_t avcodec_decode_function);
    virtual ~decode_context();
};

#endif
