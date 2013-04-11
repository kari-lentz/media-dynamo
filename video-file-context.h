#ifndef ENCODER_CONTEXT_H
#define ENCODER_CONTEXT_H

#include "decode-context.h"
#include <SDL/SDL.h>

using namespace std;

class video_file_context:public decode_context
{
private:
    ring_buffer_t* buffer_;
    SDL_Overlay* overlay_;

    specific_streamer<video_file_context, AME_VIDEO_FRAME> functor_;

    AVFormatContext* oc_;
    FILE* outfile_;

    void scale_frame(AME_VIDEO_FRAME* frame);
    void scale_frame2(AME_VIDEO_FRAME* frame);
    int decode_frames(AME_VIDEO_FRAME* frames, int size);

public:

    video_file_context(const char* mp4_file_path, ring_buffer_t* ring_buffer, SDL_Overlay* overlay);
    ~video_file_context();

    void operator()(int start_at = 0);
};

#endif
