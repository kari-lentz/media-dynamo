#ifndef RENDER_VIDEO_H
#define RENDER_VIDEO_H

#include "render.h"
#include "ring-buffer-video.h"
#include <SDL/SDL.h>

typedef struct
{
    ring_buffer_video_t* ring_buffer;
    SDL_Overlay* overlay;
    bool debug_p;
    int ret;
} env_render_video_context;

class render_video:public render<AME_VIDEO_FRAME>
{
private:

    ring_buffer_video_t* pbuffer_;
    SDL_Overlay* overlay_;

    specific_streamer<render_video, AME_VIDEO_FRAME> functor_;
    SDL_Surface* screen_;

protected:
    bool render_frame_specific(AME_VIDEO_FRAME* pframe);
    uint32_t get_media_ms();

public:

    render_video(ring_buffer_video_t* pbuffer, SDL_Overlay* overlay);
    ~render_video();

    int operator()();
};

#endif
