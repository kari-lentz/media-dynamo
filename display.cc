#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include "display.h"

using namespace std;

logger_t display::logger("display");

const char* COUNTS_MANAGER = "COUNTS-MANAGER";

int display::display_frame(AME_VIDEO_FRAME* pframes, int num_frames)
{
    //find the lowest acceptable pts and play that
    //
    int lowest_ms = 60000 * 1440;
    AME_VIDEO_FRAME* pframe_playable;

    for( int idx = 0; idx < num_frames; ++idx )
    {
        AME_VIDEO_FRAME* pframe = &pframes[ idx ];

        if(pframe->pts_ms < media_ms_)
        {
            pframe->skipped_p = true;
        }
        else
        {
            if( !pframe->skipped_p && !pframe->played_p && pframe->pts_ms <= lowest_ms )
            {
                lowest_ms = pframe->pts_ms;
                pframe_playable = pframe;
            }
        }
    }

    //populate the data with a playable frame in the ring buffer if it exists
    //
    if( pframe_playable )
    {
        //ensure overlays and surfaces created
        //
        if( !screen_ )
        {
            screen_ = SDL_SetVideoMode(pframe_playable->width, pframe_playable->height, 0, 0);
            if(!screen_)
            {
                stringstream ss;
                ss << "SDL: could not set video mode - exiting";
                throw app_fault( ss.str().c_str() );
            }
        }

        if( !overlay_ )
        {
            overlay_ = SDL_CreateYUVOverlay(pframe_playable->width, pframe_playable->height, SDL_YV12_OVERLAY, screen_);

            if( !overlay_ )
            {
                stringstream ss;
                ss << "SDL: could not create UYV overlay";
                throw app_fault( ss.str().c_str() );
            }
        }

        SDL_LockYUVOverlay( overlay_ );

        overlay_->pitches[ 0 ] = pframe_playable->linesize[0];
        overlay_->pitches[ 1 ] = pframe_playable->linesize[2];
        overlay_->pitches[ 2 ] = pframe_playable->linesize[1];

        overlay_->pixels[ 0 ] = pframe_playable->data[0];
        overlay_->pixels[ 1 ] = pframe_playable->data[2];
        overlay_->pixels[ 2 ] = pframe_playable->data[1];

        pframe_playable->played_p = true;

        SDL_UnlockYUVOverlay( overlay_ );

        SDL_Rect rect;

        rect.x = 0;
        rect.y = 0;
        rect.w = pframe_playable->width;
        rect.h = pframe_playable->height;

        SDL_DisplayYUVOverlay(overlay_, &rect);
        media_ms_ = pframe_playable->pts_ms;
    }

    int ret = 0;

    //discard first contiguous skipped and played frames in buffer
    //
    for( int idx = 0; idx < num_frames; ++idx )
    {
        AME_VIDEO_FRAME* pframe = &pframes[ idx ];

        if( pframe->played_p || pframe->skipped_p )
        {
            ++ret;
        }
    }

    return ret;
}

int display::call(AME_VIDEO_FRAME* pframes, int num_frames)
{
    try
    {
        return display_frame(pframes, num_frames);
    }
    catch( app_fault& e )
    {
        logger << e;
        return -1;
    }
}

display::display(ring_buffer_t* pbuffer):media_ms_(0), pbuffer_(pbuffer), functor_(this, &display::call), screen_(NULL), overlay_(NULL)
{
}

display::~display()
{
}

int display::operator()()
{
    int ret;
    media_ms_ = 0;

    do
    {
        ret = pbuffer_->read_avail( &functor_ );

        int delay = 20;
        usleep( delay * 1000 );
        media_ms_ = media_ms_ + delay;

    }  while( ret >= 0);

    return ret;
}
