#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include <stdint.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>

#include "display.h"
#include "decode-context.h"
#include "sdl-holder.h"

using namespace std;

logger_t display::logger("VIDEO-PLAYER");
extern lock_t<bool> global_done;

int display::display_frame(AME_VIDEO_FRAME* pframes, int num_frames)
{
    if( error_p_ ) return -1;

    //find the lowest acceptable pts and play that
    //typically the highest out of the ones that are lower.
    //
    int highest_ms = 0;
    AME_VIDEO_FRAME* pframe_playable = NULL;
    int media_ms = (int) (SDL_GetTicks() - begin_tick_ms_);

    for( int idx = 0; idx < num_frames; ++idx )
    {
        AME_VIDEO_FRAME* pframe = &pframes[ idx ];

        if(pframe->pts_ms <= media_ms)
        {
            pframe->skipped_p = true;

            if( !pframe->played_p && pframe->pts_ms >= highest_ms )
            {
                highest_ms = pframe->pts_ms;
                pframe_playable = pframe;
            }
        }
    }

    //populate the data with a playable frame in the ring buffer if it exists
    //
    if( pframe_playable )
    {
        SDL_LockYUVOverlay( overlay_ );

        memcpy(overlay_->pixels[ 0 ], pframe_playable->data[ 0 ], overlay_->pitches[0] * overlay_->h);
        memcpy(overlay_->pixels[ 2 ], pframe_playable->data[ 1 ], (overlay_->pitches[2] * overlay_->h) >> 1);
        memcpy(overlay_->pixels[ 1 ], pframe_playable->data[ 2 ], (overlay_->pitches[1] * overlay_->h) >> 1);

        pframe_playable->played_p = true;

        SDL_UnlockYUVOverlay( overlay_ );

        SDL_Rect rect;

        rect.x = 0;
        rect.y = 0;
        rect.w = overlay_->w;
        rect.h = overlay_->h;

        int ret = SDL_DisplayYUVOverlay(overlay_, &rect);

        if( ret != 0 )
        {
            stringstream ss;
            ss << "SDL: could not display YUV overlay";
            throw app_fault( ss.str().c_str() );
        }

        // caux << "played frame at media ms:" << media_ms << ":pts_ms:" << pframe_playable->pts_ms << ":num_frames:" << num_frames << endl;
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
        else
        {
            break;
        }
    }

    //caux << "processed " << ret << " frames" << endl;

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

display::display(ring_buffer_t* pbuffer, SDL_Overlay* overlay):pbuffer_(pbuffer), functor_(this, &display::call), screen_(NULL), overlay_(overlay), error_p_(false)
{
}

display::~display()
{
    display::logger << "display destroyed" << endl;
}

int display::operator()()
{
    int ret;
    begin_tick_ms_ = SDL_GetTicks();
    error_p_ = false;

    do
    {
        ret = pbuffer_->read_avail( &functor_ );

        int delay = 20;
        usleep( delay * 1000 );

        if( sdl_holder::done )
        {
            error_p_ = true;
        }

    }  while( ret >= 0 && !pbuffer_->is_eof());

    return ret;
}
