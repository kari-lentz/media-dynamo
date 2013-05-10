#include "sdl-holder.h"

logger_t sdl_holder::logger("VIDEO-PLAYER");

sdl_holder::sdl_holder(int width, int height, int num_audio_zones, list<pthread_t*>& threads):num_audio_zones_(num_audio_zones), threads_(threads), surface_(0), overlay_(0)
{
    surface_ = SDL_SetVideoMode(width, height, 0, SDL_HWSURFACE | SDL_RESIZABLE | SDL_ASYNCBLIT | SDL_HWACCEL);
    if(!surface_)
    {
        stringstream ss;
        ss << "SDL: could not set video mode - exiting";
        throw app_fault( ss.str().c_str() );
    }

    overlay_ = SDL_CreateYUVOverlay(width, height, SDL_YV12_OVERLAY, surface_);

    if( !overlay_ )
    {
        stringstream ss;
        ss << "SDL: could not create YUV overlay";
        throw app_fault( ss.str().c_str() );
    }
}

sdl_holder::~sdl_holder()
{
    logger << "will wait for secondary thread completion" << endl;

    for(list<pthread_t*>::iterator it = threads_.begin(); it != threads_.end(); ++it)
    {
        pthread_join( **it, NULL );
    }

    logger << "all secondary threads down" << endl;

    if( overlay_ ) SDL_FreeYUVOverlay( overlay_ );
    if( surface_ ) SDL_FreeSurface( surface_ );

    logger << "sdl-holder destroyed" << endl;
}

SDL_Overlay* sdl_holder::get_overlay()
{
    return overlay_;
}

SDL_Surface* sdl_holder::get_surface()
{
    return surface_;
}

void sdl_holder::message_loop(ring_buffer_video_t* ring_buffer_video, ring_buffer_audio_t* ring_buffers_audio, ready_synch_t* buffer_ready, ready_synch_t* video_ready, ready_synch_t* audio_ready)
{
    SDL_Event event;
    bool done_p = false;

    for( ;!done_p; )
    {
        SDL_WaitEvent(&event);

        switch (event.type)
        {
        case SDL_KEYDOWN:
            logger << "The " << SDL_GetKeyName(event.key.keysym.sym) << "  key was pressed!" << endl;
            break;
        case MY_DONE:
        case SDL_QUIT:
            logger << "The primary quit event detected" << endl;
            ring_buffer_video->close_out();

            for(int idx = 0; idx < num_audio_zones_; ++idx)
            {
                (&ring_buffers_audio[ idx ])->close_out();
            }

            buffer_ready->broadcast( false );
            video_ready->signal( false );
            audio_ready->signal( false );

            done_p = true;
            break;
        }
    }
}

