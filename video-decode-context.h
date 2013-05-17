#ifndef VIDEO_DECODE_CONTEXT_H
#define VIDEO_DECODE_CONTEXT_H

#include "decode-context.h"
#include "ring-buffer-video.h"
#include "vwriter.h"
#include "synch.h"
#include <SDL/SDL.h>

using namespace std;

typedef struct
{
    const char* mp4_file_path;
    int start_at;
    ring_buffer_video_t* ring_buffer;
    SDL_Overlay* overlay;
    ready_synch_t* video_primed;
    bool debug_p;
    bool run_p;
    int ret;
} env_video_decode_context;

class video_decode_context:public decode_context<AME_VIDEO_FRAME>
{
private:
    ring_buffer_video_t* buffer_;
    SDL_Overlay* overlay_;
    ready_synch_t* video_primed_;

    AVFormatContext* oc_;
    FILE* outfile_;

    void buffer_primed();
    void write_frame(AVFrame* frame_in);
    void test_cairo( AME_MIXER_FRAME* frame );

public:

    video_decode_context(const char* mp4_file_path, ring_buffer_video_t* ring_buffer, SDL_Overlay* overlay, ready_synch_t* video_primed);
    ~video_decode_context();

    void operator()(int start_at = 0);
};

#endif
