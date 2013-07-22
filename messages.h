#ifndef MESSAGES_H
#define MESSAGES_H

#include <SDL/SDL.h>

#define MY_DONE (SDL_USEREVENT + 0)
#define MY_AUDIO_PRIMED_0 (SDL_USEREVENT + 1)
#define MY_AUDIO_PRIMED_1 (SDL_USEREVENT + 2)
#define MY_AUDIO_PRIMED_2 (SDL_USEREVENT + 3)
#define MY_AUDIO_PRIMED_3 (SDL_USEREVENT + 4)
#define MY_VIDEO_PRIMED (SDL_USEREVENT + 5)
#define MY_AUDIO_BUFFERS_READY (SDL_USEREVENT + 6)

extern void post_message(int message );

#endif
