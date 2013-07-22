#include "messages.h"

void post_message(int message )
{
    SDL_Event event;
    event.type = message;
    SDL_PushEvent(&event);
}
