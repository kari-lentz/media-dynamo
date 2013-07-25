#ifndef DECODER_INST_H
#define DECODER_INST_H

#include "smart-ptr.h"
#include "result.h"
#include "decoder-result.h"
#include "ring-buffer-audio.h"
#include <openssl/rc4.h>
#include <fstream>
#include <queue>
#include <mysql++/mysql++.h>
#include "music.h"
#include "my-audio-frame.h"
#include "mp3-decode-context.h"

typedef queue< pmusic_t > base_music_queue;

class music_queue_t:public base_music_queue
{
    mysqlpp::Connection* pconn_;
    int nchannel_;
    time_t  dt_max_eom_; //maximum for which there is a memory list
    time_t dt_last_empty_check_;
    int buffer_lead_samples_;

public:
music_queue_t(env_mp3_decode_context* pzone_env):base_music_queue(), pconn_( pzone_env->conn ), nchannel_( pzone_env->channel )
    {
        time_t dt_now;

        time( &dt_now );

        dt_max_eom_ = dt_now;
        dt_last_empty_check_ = dt_now - 120;

        buffer_lead_samples_ =  pzone_env->ring_buffer->get_frames_per_period() *  pzone_env->ring_buffer->get_periods();
    }

    bool empty_check_due(bool first_connect_p)
    {
        time_t dt_now;
        time( &dt_now );

        int interval = first_connect_p ? 60 : 2; // try every 2 seconds for first connect, 60 thereafter

        bool ret = ( dt_now - dt_last_empty_check_ >= interval ) ? true : false;

        if( ret ) dt_last_empty_check_ = dt_now;

        return ret;
    }

    time_t get_max_eom_lead()
    {
        time_t dt_now;
        time( &dt_now );

        return dt_max_eom_ - dt_now;
    }

    void push( pmusic_t pmusic )
    {
        base_music_queue::push( pmusic );

        pmusic->update_status( music::eQueued, false );

        if( pmusic->get_scheduled_eom() > dt_max_eom_ )
	{
            dt_max_eom_ = pmusic->get_scheduled_eom();
	}
    }

    void pop_to_play(list<pmusic_t>& lst_playing, int lead)
    {
        pmusic_t pmusic;
        pmusic_t pmusic_primed;
        time_t dt_lead;

        time( &dt_lead );
        dt_lead += lead;

        while( !base_music_queue::empty() )
        {
            pmusic = front();

            if( pmusic->get_status() == music::eQueued && pmusic->get_scheduled_eom() < dt_lead  )
            {
                pop();
                pmusic->update_status( music::eSkipped );
            }
            else
            {
                if( pmusic->get_scheduled_start() <= dt_lead )
                {
                    lst_playing.push_back( pmusic );

                    pmusic->set_lead( ( dt_lead - pmusic->get_scheduled_start() ) * 1000 );
                    pmusic->update_status( music::ePrimed );

                    pop();
                }
                else
                {
                    break;
                }
            }
        }
    }
};

class decoder_inst
{
    music_queue_t music_queue_;

    env_mp3_decode_context* pzone_env_;

    // zone specific environent variables
    mysqlpp::Connection* pconn_;
    int nchannel_;
    ring_buffer_audio_t* pbuffer_;
    short* pbuffertemp_;
    short* pbufferonholdmask_;
    short* pbuffersamples_;
    bool bsawskipped_;
    bool first_connect_p_;
    int pts_ms_;

    void restock_music_queue();

    size_t read_avail(unsigned char* prawbuffer,size_t bytes);
    int call(AME_AUDIO_FRAME* prawbuffer, int nwords);

    list<pmusic_t> music_playing_;

    static logger_t logger;

public:

    static bool is_played(const pmusic_t& pmusic);

    decoder_inst(env_mp3_decode_context* pzone_env );

    ~decoder_inst();

    int operator()();
};

#endif


