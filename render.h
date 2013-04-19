#ifndef RENDER_H
#define RENDER_H

#include <stdio.h>
#include <stdint.h>
#include <SDL/SDL.h>
#include "app-fault.h"
#include "ring-buffer.h"
#include "logger.h"

using namespace std;

template <typename T> class render
{
private:

    Uint32* pbegin_tick_ms_;

protected:

    virtual bool render_frame_specific(T* pframe)=0;

    int render_frames(T* pframes, int num_frames)
    {
        int ret = 0;

        try
        {
            if(!pbegin_tick_ms_)
            {
                pbegin_tick_ms_ = new Uint32;
                *pbegin_tick_ms_ = SDL_GetTicks();
            }

            //find the lowest acceptable pts and play that
            //typically the highest out of the ones that are lower.
            //
            int highest_ms = 0;
            T* pframe_playable = NULL;
            int media_ms = (int) (SDL_GetTicks() - *pbegin_tick_ms_);

            for( int idx = 0; idx < num_frames; ++idx )
            {
                T* pframe = &pframes[ idx ];

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
                if( render_frame_specific(pframe_playable)) pframe_playable->played_p = true;
            }

            //discard first contiguous skipped and played frames in buffer
            //
            for( int idx = 0; idx < num_frames; ++idx )
            {
                T* pframe = &pframes[ idx ];

                if( pframe->played_p || pframe->skipped_p )
                {
                    ++ret;
                }
                else
                {
                    break;
                }
            }
        }
        catch( app_fault& e )
        {
            logger << e;
            return -1;
        }


        return ret;
    }

public:

    static logger_t logger;

render():pbegin_tick_ms_(NULL){}

    ~render()
    {
        if(pbegin_tick_ms_) delete pbegin_tick_ms_;
        render<T>::logger << "display destroyed" << endl;
    }

};

#endif
