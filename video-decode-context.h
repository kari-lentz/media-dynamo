#ifndef VIDEO_DECODE_CONTEXT_H
#define VIDEO_DECODE_CONTEXT_H

#include "decode-context.h"
#include "ring-buffer-video.h"
#include "vwriter.h"
#include <SDL/SDL.h>

using namespace std;

typedef struct
{
    const char* mp4_file_path;
    int start_at;
    ring_buffer_video_t* ring_buffer;
    SDL_Overlay* overlay;
    bool debug_p;
    int ret;
} env_video_decode_context;

class video_decode_context:public decode_context<AME_VIDEO_FRAME>
{
private:
    ring_buffer_video_t* buffer_;
    SDL_Overlay* overlay_;

    AVFormatContext* oc_;
    FILE* outfile_;

    void write_frame(AVFrame* frame_in, AME_VIDEO_FRAME* frame_out);

public:

    video_decode_context(const char* mp4_file_path, ring_buffer_video_t* ring_buffer, SDL_Overlay* overlay);
    ~video_decode_context();

    void operator()(int start_at = 0);
};

#endif
