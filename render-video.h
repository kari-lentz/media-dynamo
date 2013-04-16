#ifndef RENDER_VIDEO_H
#define RENDER_VIDEO_H

#include "render.h"

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

    SDL_Surface* screen_;
    SDL_Overlay* overlay_;

protected:
    int render_frame_specific(AME_VIDEO_FRAME* pframe);

public:

    render_video(ring_buffer_video_t* pbuffer, SDL_Overlay* overlay);
    ~render_video();
};

#endif
