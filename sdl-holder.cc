#include <cairo/cairo.h>
#include "sdl-holder.h"

logger_t sdl_holder::logger("VIDEO-PLAYER");

bool sdl_holder::render_frame_specific(AME_VIDEO_FRAME* pframe)
{
    int depth = 32;

    Uint32 rmask = 0x00FF0000;
    Uint32 gmask = 0x0000FF00;
    Uint32 bmask = 0x000000FF;
    Uint32 amask = 0x00000000;

    SDL_Surface* mem_surface = SDL_CreateRGBSurfaceFrom(pframe->raw_data, pframe->width, pframe->height, depth, surface_->pitch, rmask, gmask, bmask, amask);

    if( !mem_surface )
    {
        stringstream ss;
        ss << "SDL: could ceate memory surface";
        throw app_fault( ss.str().c_str() );
    }

    int ret = SDL_BlitSurface(mem_surface, NULL, surface_, NULL);

    if( ret != 0 )
    {
        stringstream ss;
        ss << "SDL: could not display YUV overlay";
        throw app_fault( ss.str().c_str() );
    }

    // SDL_UpdateRect(surface_, 0, 0, 0, 0);
    SDL_Flip(surface_);

    if(mem_surface) SDL_FreeSurface( mem_surface );


    return ret == 0;
}

uint32_t sdl_holder::get_media_ms()
{
    return SDL_GetTicks();
}

sdl_holder::sdl_holder(int num_audio_zones, list<pthread_t*>& threads):render<AME_VIDEO_FRAME>( "VIDEO-PLAYER"), num_audio_zones_(num_audio_zones), threads_(threads), surface_(NULL),functor_(this, &sdl_holder::render_earliest_frame)
{
}

sdl_holder::~sdl_holder()
{
    if( surface_ ) SDL_FreeSurface( surface_ );

    logger << "DESTROYED CAIRO SURFACE" << endl;

    for(list<pthread_t*>::iterator it = threads_.begin(); it != threads_.end(); ++it)
    {
        pthread_join( **it, NULL );
    }

    logger << "all secondary threads down" << endl;

    logger << "sdl-holder destroyed" << endl;
}

void sdl_holder::message_loop(ring_buffer_video_t* ring_buffer_video, ring_buffer_audio_t* ring_buffers_audio, ready_synch_t* buffer_ready, ready_synch_t* audio_ready)
{

    logger << "CREATING THE SURFACES" << endl;

/* ... make sure sdlsurf is locked or doesn't need locking ... */

    surface_ = SDL_SetVideoMode(ring_buffer_video->width_, ring_buffer_video->height_, 0, SDL_RESIZABLE | SDL_ASYNCBLIT | SDL_HWACCEL | SDL_DOUBLEBUF);

    if(!surface_)
    {
        stringstream ss;
        ss << "SDL: could not set video mode - exiting";
        throw app_fault( ss.str().c_str() );
    }

    SDL_Event event;
    bool done_p = false;
    bool video_primed_p = false;
    bool audio_zone_primed_p[ 4 ] = {false, false, false, false};
    bool audio_primed_p = false;

    for( ;!done_p; )
    {
        if(audio_primed_p && video_primed_p)
        {
            ring_buffer_video->read_avail( &functor_ );

            if( ring_buffer_video->is_done() )
            {
                logger << "closing out audio buffers" << endl;

                for(int idx = 0; idx < num_audio_zones_; ++idx)
                {
                    (&ring_buffers_audio[ idx ])->close_out();
                }

                done_p = true;
            }
        }
        else if( !audio_primed_p )
        {
            if( audio_zone_primed_p[ 0 ] && audio_zone_primed_p[ 1 ] && audio_zone_primed_p[ 2 ] && audio_zone_primed_p[ 3 ]  )
            {
                logger << "ALL AUDIO NOW PRIMED" << endl;

                audio_primed_p = true;
                audio_ready->broadcast( true );
            }
        }

        if( SDL_PollEvent(&event) > 0 )
        {
            switch (event.type)
            {
            case SDL_KEYDOWN:
            {
                logger << "The " << SDL_GetKeyName(event.key.keysym.sym) << "  key was pressed!" << endl;
                break;
            }
            case MY_AUDIO_PRIMED_0:
            {
                logger << "ZONE 1 AUDIO NOW PRIMED" << endl;

                audio_zone_primed_p[0] = true;
                break;
            }
            case MY_AUDIO_PRIMED_1:
            {
                logger << "ZONE 2 AUDIO NOW PRIMED" << endl;

                audio_zone_primed_p[1] = true;
                break;
            }
            case MY_AUDIO_PRIMED_2:
            {
                logger << "ZONE 3 AUDIO NOW PRIMED" << endl;

                audio_zone_primed_p[2] = true;
                break;
            }
            case MY_AUDIO_PRIMED_3:
            {
                logger << "ZONE 4 AUDIO NOW PRIMED" << endl;

                audio_zone_primed_p[3] = true;
                break;
            }
            case MY_VIDEO_PRIMED:
            {
                logger << "VIDEO NOW PRIMED" << endl;

                video_primed_p = true;
                break;
            }
            case MY_AUDIO_BUFFERS_READY:
            {
                logger << "all audio buffers ready" << endl;

                buffer_ready->broadcast(true);
                break;
            }
            case MY_DONE:
            case SDL_QUIT:
                logger << "The primary quit event detected" << endl;
                ring_buffer_video->close_out();

                break;
            }
        }

        int delay = 10;
        usleep( delay * 1000 );
    }
}

