#ifndef SDL_HOLDER
#define SDL_HOLDER

#include <SDL/SDL.h>
#include "app-fault.h"
#include "logger.h"
#include "thread-lock.h"

class sdl_holder
{
private:

    pthread_t* pthread_fc_;
    pthread_t* pthread_display_;

    SDL_Surface* surface_;
    SDL_Overlay* overlay_;

public:

    sdl_holder(int width, int height, pthread_t* pthread_fc_, pthread_t* pthread_display_);
    ~sdl_holder();

    static logger_t logger;
    static lock_t<bool> done;

    SDL_Overlay* get_overlay();
    SDL_Surface* get_surface();

    void message_loop();
};

#endif
