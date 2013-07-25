#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include <stdint.h>
#endif

#include <time.h>
#include "smart-ptr.h"
#include "alsa-engine.h"
#include "mpthree-iter.h"
#include "decoder-inst.h"
#include "video-player.h"

logger_t decoder_inst::logger("AUDIO-PLAYER");

void decoder_inst::restock_music_queue()
{
    if( music_queue_.get_max_eom_lead() <= QUEUE_LEAD_TIME_MIN && music_queue_.empty_check_due(first_connect_p_) )
    {
        if( !pconn_->connected() || !pconn_->ping() )
	{
            const char* db = getenv( "DB_NAME" );
            const char* server = "127.0.0.1";
            const char* user = getenv( "DB_USER" );
            const char* pass = getenv( "DB_PASSWORD" );

            if ( pconn_->connect(db, server, user, pass ) )
	    {
                logger << "connected succcessfully to database" << endl;
	    }
            else
	    {
                logger << "DB connection failed: " << pconn_->error() << endl;
	    }
	}

        // Retrieve the next 4 hours of music into memory
        //
        time_t dt_now;
        time( &dt_now );

        time_t dt_temp;
        struct tm * timeinfo;

        dt_temp = music_queue_.get_max_eom_lead() > 0 ? music_queue_.get_max_eom_lead() + dt_now : dt_now;
        string stime_lo = mysqlpp::DateTime( dt_temp );

        dt_temp = dt_now + QUEUE_LEAD_TIME_MAX;
        string stime_hi = mysqlpp::DateTime( dt_temp );

        if( pconn_->connected() )
	{
            first_connect_p_ = true;

            stringstream sssql, sssql_where;
            sssql << "select music_id, item_type, seq_num, scheduled_start, scheduled_eom, volume_level, background_volume_adjustment from AME_PROFILE_PLAY_LIST where ";

            sssql_where << "status is not null and ";
            sssql_where << "scheduled_eom > '" << stime_lo << "' and ";
            sssql_where << "scheduled_eom < '" << stime_hi << "' and ";
            sssql_where << "item_type in ('MIX','SPOT','ONHOLD') and ZONE_ID = " << (nchannel_ + 1) << " order by scheduled_start";

            string sql_where = sssql_where.str();
            sssql << sql_where;

            mysqlpp::Query query = pconn_->query( sssql.str() );
            if (mysqlpp::StoreQueryResult res = query.store())
	    {
                mysqlpp::StoreQueryResult::const_iterator it;
                for (it = res.begin(); it != res.end(); ++it )
		{
                    mysqlpp::Row row = *it;

                    int music_id = row[ 0 ];
                    string item_type = mysqlpp::String( row[1] ).c_str();
                    int seq_num = row[ 2 ];
                    time_t scheduled_start = mysqlpp::DateTime( row[ 3 ] );
                    time_t scheduled_eom = mysqlpp::DateTime( row[ 4 ] );
                    int volume_level = ( row[ 5 ] != mysqlpp::null ? row[ 5 ] : 0 );
                    int background_level = ( row[ 6 ] != mysqlpp::null ? row[ 6 ] : 0 );

                    music_queue_.push( new music(pzone_env_, music_id, item_type == "ONHOLD", seq_num, scheduled_start, scheduled_eom, volume_level, background_level ) );
		}

                new (&sssql) stringstream;
                sssql << "update AME_PROFILE_PLAY_LIST set status = 'QUEUED' where ";
                sssql << sql_where;

                if( !pconn_->query().execute( sssql.str() ) )
		{
                    logger << "failed to bulk update queued music because of " << pconn_->error() << endl;;
		}
	    }
            else
	    {
                logger << "Failed to select from playlist: " << pconn_->error() << endl;
	    }

            int ntemp = music_queue_.get_max_eom_lead() / 60;
	    if( ntemp >= 0 )
            {
		logger << "music queue has lead of " << ntemp << " minutes" << endl;
            }

            if( !bsawskipped_ )
	    {
                dt_temp = dt_now - 300;
                string stime_lo_skip = mysqlpp::DateTime( dt_temp );

                new (&sssql) stringstream;

                sssql << "update AME_PROFILE_PLAY_LIST set status = 'SKIPPED' where ";
                sssql << "scheduled_eom < '" << stime_lo_skip << "' and ";
                sssql << "(status not in ('PLAYED') or status is NULL) and ";
                sssql << "item_type = 'MIX' and ZONE_ID = " << (nchannel_ + 1) << " order by scheduled_start";

                if( !pconn_->query().execute( sssql.str() ) )
		{
                    logger << "failed to bulk update skipped music because of " << pconn_->error() << endl;;
		}
                else
		{
                    bsawskipped_ = true;
		}
	    }
	}
    }
}

decoder_inst::decoder_inst(env_mp3_decode_context* pzone_env ):music_queue_( pzone_env ), pzone_env_(pzone_env), pconn_( pzone_env->conn ), nchannel_( pzone_env->channel), pbuffer_(pzone_env->ring_buffer), pbuffertemp_(NULL), pbufferonholdmask_(NULL), pbuffersamples_(NULL), bsawskipped_(false), first_connect_p_( false ), pts_ms_(0)
{
    pbuffertemp_ = new short[ pbuffer_->get_periods() * pbuffer_->get_frames_per_period() * AUDIO_PACKET_SIZE ];
    pbufferonholdmask_ = new short[ pbuffer_->get_periods() * pbuffer_->get_frames_per_period() * AUDIO_PACKET_SIZE ];
    pbuffersamples_ = new short[ pbuffer_->get_periods() * pbuffer_->get_frames_per_period() * AUDIO_PACKET_SIZE ];
}

decoder_inst::~decoder_inst()
{
    delete [] pbuffertemp_;
    delete [] pbufferonholdmask_;
    delete [] pbuffersamples_;
}

// a predicate implemented as a function:
bool decoder_inst::is_played(const pmusic_t& pmusic)
{
    return (pmusic->get_status() == music::ePlayed);
}

size_t decoder_inst::read_avail(unsigned char* prawbuffer, size_t bytes)
{
    size_t bytes_read, words_read;
    size_t max_bytes_read = 0;
    pmusic_t pmusic;
    short *prawbuffer_words = (short*) prawbuffer;

    restock_music_queue();
    //int lead_seconds = SAMPLES_TO_SECONDS( pbuffer_->get_available_samples() ) * (video_player::PERIODS_ALSA + video_player::PERIODS_MPEG) / player::PERIODS_MPEG;
    int lead_seconds = 0;
    music_queue_.pop_to_play( music_playing_, lead_seconds );

#ifdef DEBUG0
    logger << "playing music with lead of " << lead_seconds << "s" << endl;
#endif

    //pop any music that is in eof in terms of run time
    //
    music_playing_.remove_if( &decoder_inst::is_played );

    memset( prawbuffer, 0, bytes );
    memset( pbufferonholdmask_, 0, bytes ); //modified in place in loop

    // filter out onhold tracks from list, buffering them and calculating onhold mask as side effect
    //
    for( list<pmusic_t>::iterator it = music_playing_.begin(); it != music_playing_.end(); ++it )
    {
        pmusic = *it;

        if( pmusic->is_onhold() )
	{
            bytes_read = pmusic->read_pcm( (unsigned char*) pbuffertemp_, bytes );
            size_t words_read = bytes_read >> 1;

            for( int idx = 0; idx < words_read; idx++ )
	    {
                prawbuffer_words[ idx ] += pbuffertemp_[ idx ] * pmusic->get_volume_level() / 100;
                pbufferonholdmask_[ idx ] =+ pmusic->get_background_volume_adjustment();  //in place delta
	    }

            if (bytes_read > max_bytes_read) max_bytes_read = bytes_read;
	}
    }

    // filter out nononhold tracks from list, buffering them according to onhold mask
    //
    for( list<pmusic_t>::iterator it = music_playing_.begin(); it != music_playing_.end(); ++it )
    {
        pmusic = *it;

        if( !pmusic->is_onhold() )
	{

            bytes_read = pmusic->read_pcm( (unsigned char*) pbuffertemp_, bytes );
            size_t words_read = bytes_read >> 1;

            for( int idx = 0; idx < words_read; idx++ )
	    {
                prawbuffer_words[ idx ] += pbuffertemp_[ idx ] * (pmusic->get_volume_level() + pbufferonholdmask_[idx]) / 100;
	    }

            if (bytes_read > max_bytes_read) max_bytes_read = bytes_read;
	}
    }

    return max_bytes_read;
}

int decoder_inst::call(AME_AUDIO_FRAME* prawbuffer, int frames)
{
    //this function must report the proper number of audio bytes, even if zero's are punched in
    //
    int nsamples = AUDIO_PACKET_SIZE * frames;
    int total_bytes = nsamples * 2;
    unsigned char* ppcm = (unsigned char*)pbuffersamples_;

    while( total_bytes > 0 )
    {
        size_t num_bytes;
        num_bytes = read_avail( ppcm + 2 * nsamples - total_bytes, total_bytes );
        if( num_bytes <= 0 )
	{
#ifdef DEBUG0
            logger << "0 bytes read ... bailing" << endl;
#endif
            break;
	}
        total_bytes -= num_bytes;
    }

    // line below only needed for error handling
    //
    for( int nbyte = 2 * nsamples - total_bytes; nbyte < total_bytes; nbyte++ )
    {
        ( (unsigned char*)pbuffersamples_ )[nbyte] = 0;
    }

    int total_sample_idx = 0;
    int frame_ctr = 0;

    for( int frame_idx = 0 ; frame_idx < frames; ++frame_idx )
    {
        if(total_sample_idx >= nsamples) break;

        frame_ctr++;
        AME_AUDIO_FRAME* pframe = &prawbuffer[ frame_idx ];
        pframe->samples = 0;
        pframe->played_p = false;
        pframe->skipped_p = false;

        for(int sample_idx = 0; sample_idx < AUDIO_PACKET_SIZE; ++sample_idx)
        {
            if(total_sample_idx >= nsamples) break;
            ++pframe->samples;
            pframe->raw_data[ sample_idx ][0] = pbuffersamples_[ total_sample_idx ];
            ++total_sample_idx;
        }

        pframe->pts_ms = pts_ms_;
        pts_ms_ += (pframe->samples * 1000 / 44100);
        //if( pts_ms_ > (1024 * 1024) ) pts_ms_ = 0;

        //logger_ << "FRAME_PTS:" << pts_ms_ << "SAMPLES:" << (pframe->samples * 1000 / 44100) << endl;
    }

#ifdef DEBUG0
    logger << "  " << nwords - ( total_bytes >> 1 ) << " samples sent " << endl;
#endif

    return frame_ctr;
}

int decoder_inst::operator()()
{
    specific_streamer< decoder_inst, AME_AUDIO_FRAME > pfn_write( this, &decoder_inst::call );

    // main loop
    //
    while( 1 )
    {
        pbuffer_->write_period( &pfn_write );
    }

    if( pconn_->connected() )
    {
        pconn_->disconnect();
    }

    return 0;
};
