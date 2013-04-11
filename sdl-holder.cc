#include "sdl-holder.h"

logger_t sdl_holder::logger("VIDEO-PLAYER");
lock_t<bool> sdl_holder::done;

sdl_holder::sdl_holder(int width, int height, pthread_t* pthread_fc, pthread_t* pthread_display):pthread_fc_(pthread_fc),pthread_display_(pthread_display),surface_(0),overlay_(0)
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
    logger << "will wait for secondary thread completion:last ret:" << endl;
\
    pthread_join( *pthread_display_, NULL );
    pthread_join( *pthread_fc_, NULL );
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

void sdl_holder::message_loop()
{
    SDL_Event event;

    while( !sdl_holder::done )
    {
        SDL_WaitEvent(&event);

        switch (event.type)
        {
        case SDL_KEYDOWN:
            logger << "The " << SDL_GetKeyName(event.key.keysym.sym) << "  key was pressed!" << endl;
            break;
        case SDL_QUIT:
            logger << "The primary quit event detected" << endl;
            sdl_holder::done = (lock_t<bool>) true;
            break;
        }
    }
}

