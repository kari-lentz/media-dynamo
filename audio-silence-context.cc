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
#include <map>
#include <string>
#include <sstream>

#include "audio-silence-context.h"

using namespace std;

void audio_silence_context::write_frame()
{
    AME_AUDIO_FRAME frame_out;

    frame_out.pts_ms = 0;
    frame_out.played_p = false;
    frame_out.skipped_p = false;

    for(int sample = 0;sample < AUDIO_PACKET_SIZE; sample++)
    {
        frame_out.raw_data[sample][0] = 0;
        frame_out.raw_data[sample][1] = 0;
    }

    frame_out.samples = AUDIO_PACKET_SIZE;

    //logger_ << "AUDIO BEGIN DECODDE FRAME at:" << frame_out.pts_ms << endl;
    int ret = vwriter<AME_AUDIO_FRAME>(buffer_, false)( &frame_out, 1);
    //logger_ << "AUDIO END DECODDE FRAME at:" << frame_out.pts_ms << ":" << ret << endl;

    if( ret <= 0 ) throw decode_done_t();

    //logger_ << "WROTE SILENCE TO AUDIO BUFFER:" << frame_out.samples << endl;
}

audio_silence_context::audio_silence_context(ring_buffer_audio_t* ring_buffer):buffer_( ring_buffer ), logger_("AUDIO-PLAYER"), min_frames_( ring_buffer->get_frames_per_period() * (ring_buffer->get_periods() - 1) )
{
}

audio_silence_context::~audio_silence_context()
{
}

void audio_silence_context::operator()()
{
    int frames = 0;
    bool primed = false;

    try
    {
        while(true)
        {
            if( !primed )
            {
                if( frames >= min_frames_ )
                {

                    primed = true;
                    post_message(buffer_->primed_message_);
                }

                ++frames;
            }

            write_frame();
        }
    }
    catch(decode_done_t& e)
    {
        vwriter<AME_AUDIO_FRAME>(buffer_, false).close();
    }
    catch(app_fault& e)
    {
        logger_ << e;
        vwriter<AME_AUDIO_FRAME>(buffer_, false).error();
    }
}



