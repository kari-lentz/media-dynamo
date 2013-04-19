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
protected:

    virtual bool render_frame_specific(T* pframe)=0;

private:

    ring_buffer<T>* pbuffer_;
    specific_streamer<render,T> functor_;
    Uint32* pbegin_tick_ms_;

    int render_frame(T* pframes, int num_frames)
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

        int ret = 0;

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

        return ret;
    }

    int call(T* pframes, int num_frames)
    {
        try
        {
            return render_frame(pframes, num_frames);
        }
        catch( app_fault& e )
        {
            logger << e;
            return -1;
        }
    }

public:

    static logger_t logger;

render(ring_buffer<T>* pbuffer):pbuffer_(pbuffer), functor_(this, &render<T>::call), pbegin_tick_ms_(NULL){}

    ~render()
    {
        if(pbegin_tick_ms_) delete pbegin_tick_ms_;
        render<T>::logger << "display destroyed" << endl;
    }

    int operator()()
    {
        do
        {
            pbuffer_->read_avail( &functor_ );

            int delay = 20;
            usleep( delay * 1000 );

        }  while( !pbuffer_->is_done() );

        return 0;
    }
};

#endif
