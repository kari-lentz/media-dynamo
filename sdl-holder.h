#ifndef SDL_HOLDER
#define SDL_HOLDER

#include <SDL/SDL.h>
#include "app-fault.h"
#include "logger.h"
#include "ring-buffer-video.h"
#include "ring-buffer-audio.h"
#include "synch.h"

#define MY_DONE (SDL_USEREVENT + 0)

class sdl_holder
{
private:

    int num_audio_zones_;

    pthread_t* pthread_vfc_;
    pthread_t* pthread_afc_;
    pthread_t* pthread_render_video_;
    pthread_t* pthread_render_audio_;

    SDL_Surface* surface_;
    SDL_Overlay* overlay_;

public:

    sdl_holder(int width, int height, int num_audio_zones, pthread_t* pthread_vfc_, pthread_t* pthread_afc_, pthread_t* pthread_render_video_, pthread_t* pthread_render_audio_);
    ~sdl_holder();

    static logger_t logger;

    SDL_Overlay* get_overlay();
    SDL_Surface* get_surface();

    void message_loop(ring_buffer_video_t* ring_buffer_video, ring_buffer_audio_t* ring_buffers_audio, ready_synch_t* buffer_ready, ready_synch_t* video_ready, ready_synch_t* audio_ready);

};

#endif
