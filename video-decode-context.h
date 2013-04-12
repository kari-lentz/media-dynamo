#ifndef VIDEO_DECODE_CONTEXT_H
#define VIDEO_DECODE_CONTEXT_H

#include "decode-context.h"
#include <SDL/SDL.h>

using namespace std;

typedef struct
{
    const char* mp4_file_path;
    int start_at;
    ring_buffer_t* ring_buffer;
    SDL_Overlay* overlay;
    bool debug_p;
    int ret;
} env_video_decode_context;

class video_decode_context:public decode_context<AME_VIDEO_FRAME>
{
private:
    ring_buffer_t* buffer_;
    SDL_Overlay* overlay_;

    specific_streamer<video_decode_context, AME_VIDEO_FRAME> functor_;

    AVFormatContext* oc_;
    FILE* outfile_;

    void write_frame2(AME_VIDEO_FRAME* frame);
    void write_frame(AME_VIDEO_FRAME* frame);
    int decode_frames(AME_VIDEO_FRAME* frames, int size);

public:

    video_decode_context(const char* mp4_file_path, ring_buffer_t* ring_buffer, SDL_Overlay* overlay);
    ~video_decode_context();

    void operator()(int start_at = 0);
};

#endif
