#ifndef RENDER_VIDEO_H
#define RENDER_VIDEO_H

#include <SDL/SDL.h>
#include "render.h"
#include "ring-buffer-video.h"
#include "synch.h"

typedef struct
{
    ring_buffer_video_t* ring_buffer;
    SDL_Overlay* overlay;
    ready_synch_t* audio_ready;
    ready_synch_t* video_ready;
    bool debug_p;
    bool run_p;
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
