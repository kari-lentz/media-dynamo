#ifndef SDL_HOLDER
#define SDL_HOLDER

#include <list>
#include <SDL/SDL.h>
#include <cairo/cairo.h>
#include "messages.h"
#include "app-fault.h"
#include "logger.h"
#include "render.h"
#include "ring-buffer-video.h"
#include "ring-buffer-audio.h"
#include "synch.h"
#include "cairo-f.h"

class sdl_holder:public render<AME_VIDEO_FRAME>
{
private:

    int num_audio_zones_;

    list<pthread_t*>& threads_;

    SDL_Surface* surface_;
    specific_streamer<sdl_holder, AME_VIDEO_FRAME> functor_;

protected:
    bool render_frame_specific(AME_VIDEO_FRAME* pframe);
    uint32_t get_media_ms();

public:

    sdl_holder(int num_audio_zones, list<pthread_t*>& threads);
    ~sdl_holder();

    static logger_t logger;

    void message_loop(ring_buffer_video_t* ring_buffer_video, ring_buffer_audio_t* ring_buffers_audio, ready_synch_t* buffer_ready, ready_synch_t* audio_ready);

};

#endif
