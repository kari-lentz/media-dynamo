#ifndef RENDER_H
#define RENDER_H

#include <stdio.h>
#include <stdint.h>
#include "app-fault.h"
#include "ring-buffer.h"
#include "logger.h"

using namespace std;

template <typename T> class render
{
protected:

    logger_t logger_;

private:

    uint32_t* pbegin_tick_ms_;

protected:

    virtual bool render_frame_specific(T* pframe)=0;
    virtual uint32_t get_media_ms()=0;

    int render_frames(T* pframes, int num_frames)
    {
        int ret = 0;

        try
        {
            if(!pbegin_tick_ms_)
            {
                pbegin_tick_ms_ = new uint32_t;
                *pbegin_tick_ms_ = get_media_ms();
            }

            //find the lowest acceptable pts and play that
            //typically the highest out of the ones that are lower.
            //
            int highest_ms = 0;
            T* pframe_playable = NULL;
            int media_ms = (int) (get_media_ms() - *pbegin_tick_ms_);

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
            logger_ << e;
            return -1;
        }


        return ret;
    }

public:

    static logger_t logger;

render(const char* logging_context):logger_(logging_context),pbegin_tick_ms_(NULL){}

    ~render()
    {
        if(pbegin_tick_ms_) delete pbegin_tick_ms_;

    }

};

#endif
