#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include <stdint.h>
#endif

#include "smart-ptr.h"
#include "music.h"
#include "alsa-engine.h"
#include "mpthree-iter.h"

using namespace std;

logger_t music::logger("AUDIO-PLAYER");

struct decode_format
{
  long rate_;
  int channels_;
  int encoding_;

  friend ostream& operator <<( ostream& os, decode_format& f )
  {
    os << "rate:" << f.rate_ << " channels:"  << f.channels_ << " encoding:" << f.encoding_;
    return os;
  }
};

const char* music::statuses_[] = { "NULL", "SKIPPED", "QUEUED", "PRIMED", "PLAYING", "PLAYED" };

void music::debug_read(packed_array<unsigned char>& buffer)
{
#ifdef DEBUG0
  logger << "  len:"  << buffer.len()  << hex << state_ << " first 4 bytes:  0x";
  for( int nidx = 0; nidx < ( buffer.len() >= 4 ? 4 : buffer.len() ); nidx++ )
    {
      unsigned long ul = buffer[ nidx ];
      logger << ul;
    }

  logger << std::dec << endl;

#endif
}

ppa_char music::calculate_file_path()
{
    stringstream sssql;
    ppa_char ppath;

    sssql << "select m.rpm_category_id, mf.file_name, mf.eom_ms, mf.file_id from AME_PROFILE_MUSIC_INSTALLED mi, AME_MUSIC m, AME_MUSIC_FILE mf where mi.music_id = m.music_id and m.music_id = mf.music_id and mi.file_id = mf.file_id ";
    sssql << "and m.music_id = " << music_id_;

    mysqlpp::Query query = pconn_->query( sssql.str() );
    if (mysqlpp::StoreQueryResult res = query.store())
    {
        mysqlpp::StoreQueryResult::const_iterator it;
        it = res.begin();
        if ( it != res.end() )
	{
            mysqlpp::Row row = *it;

            ppath = new pa_char;
            *ppath << "/mnt/MUSIC/" << row[0] << "/" << row[1];

            //reset real eom based on accurate processing value
            //
            real_eom_ = row[2] != mysqlpp::null ? row[2] : 0;

            file_id_ = row[3];
	}
      else
      {
            logger << "failed to locate file for scheduled music_id " << music_id_ << endl;
      }
    }
    else
    {
        logger << "Failed to open playlist: " << query.error() << endl;
    }

    return ppath;
}

decoder_result& music::open_file()
{
  int ierr;
  decode_format f;

  ppa_char ppath = calculate_file_path();

  if( ppath )
  {
      const char* szfile = *ppath;

      //b77005558a
#ifdef _DEBUG1
      logger << "trying to open music id:  " << music_id_ << " file: " << szfile << " state: "  << (bool) state_  << endl;
#endif

      fin_.open( szfile, ios::in | ios :: binary );

      if( fin_.is_open() && !fin_.eof() && !fin_.fail() )
      {
	  const unsigned char key0[ 5 ] = { 0xb7, 0x70, 0x05, 0x55, 0x8a };
	  unsigned char key1[ 16 ];
	  memset( key1, 0, sizeof(key1) );
	  memcpy( key1, key0, sizeof( key0 ) );
	  RC4_set_key( &key_, sizeof(key1), key1 );

	  int ierr;
	  state_( "mpg123_new", mpg123_new(NULL, &ierr), &ierr );
	  state_( "mpg123_open_feed", mpg123_open_feed( state_ ) );

	  logger << state_ <<  " playing: " << szfile << endl;
	  update_status( ePlaying );
	  insert_play_log();
	}
      else
	{
	  logger << state_ << " unsuccessfully opened file:  " << szfile << endl;
	}

#ifdef _DEBUG1
      logger << "after open file:  " << szfile << " " << fin_.is_open() << fin_.eof() << fin_.fail() << endl;
#endif
    }
  else
    {
      logger << "bad path channel:" << nchannel_ << endl;
    }

  return state_;
}

int music::read_mp3( unsigned char* pbuffer, int nbytes )
{
  int nret = 0;

  if( !is_ready() && !is_eof() )
  {
      open_file();
  }

  if( is_ready() )
  {
      fin_.read( (char*) psz_mp3_buff_raw_, nbytes );
      nret = fin_.gcount();

      if( nret > 0 )
      {
	  RC4( &key_, nret, psz_mp3_buff_raw_, pbuffer );
      }

      if( !is_ready() )
      {
	  update_status( ePlayed );
      }
  }

  return nret;
}

decoder_result& music::feed_mp3()
{
    specific_streamer<music, unsigned char> fnwriter( this, &music::read_mp3 );

    mp3_buff_whole_ = mp3_buff_remainder_;
    mp3_buff_whole_.fill_to_cap( &fnwriter );
    debug_read(mp3_buff_whole_);

    mp3_iter it( mp3_buff_whole_, mp3_buff_whole_.len() );
    int nfeed_bytes = it.get_full_frame_bytes();
    mp3_buff_remainder_.copyfrom( mp3_buff_whole_, nfeed_bytes );
    debug_read(mp3_buff_remainder_);

    state_( "mpg123_feed", mpg123_feed( state_, (const unsigned char*) mp3_buff_whole_, nfeed_bytes ) );

    if( state_ && nfeed_bytes > 0 ) bneed_data_ = false;
}

int music::raw_read_pcm(void* prawbuffer, size_t bytes, size_t* pbytes_read)
{
    size_t bytes_read = 0;

    if( bneed_data_ )
    {
        feed_mp3();
    }

    state_( "mpg123_read", mpg123_read( state_, (unsigned char*) prawbuffer, bytes, &bytes_read ) );

    if( state_ == MPG123_NEW_FORMAT )
    {
        decode_format f;
        state_( "mpg123_getformat", mpg123_getformat( state_, &f.rate_, &f.channels_, &f.encoding_ ) );

#ifdef _DEBUG1
        logger_ << f << endl;
#endif
        bneed_data_ = true;
    }
    else if( state_ == MPG123_NEED_MORE )
    {
        bneed_data_ = true;
    }

#ifdef DEBUG0
    logger_ << state_ << "bytes_needed " << bytes << " bytes read:" << bytes_read << " need data:" << bneed_data_  << endl;
#endif

    if( bytes_read > 0 )
    {
        run_.inc_bytes( bytes_read );
    }

    *pbytes_read = bytes_read;

    return state_;
}

music::music(env_mp3_decode_context* pzone_env , int music_id, bool onhold_p, int seq_num, time_t scheduled_start, time_t scheduled_eom, int volume_level, int background_level ):real_eom_( (scheduled_eom - scheduled_start) * 1000  ), bneed_data_( true ), state_(pzone_env->channel), mp3_buff_whole_( 4096 ), mp3_buff_remainder_( 4096 ), pcm_buff_seek_( 4096  ), pconn_( pzone_env->conn ), nchannel_ ( pzone_env->channel ), status_( eNULL )
{
    music_id_ = music_id;
    onhold_p_ = onhold_p;
    seq_num_ = seq_num;
    scheduled_start_ = scheduled_start;
    scheduled_eom_ = scheduled_eom;
    volume_level_ = volume_level;
    background_level_ = background_level;

    psz_mp3_buff_raw_ = new unsigned char[ mp3_buff_whole_.cap() ];
}

music::~music()
{
#ifdef _DEBUG1
    logger << "destroying music id:" << music_id_  << endl;
#endif

    if( psz_mp3_buff_raw_ )
    {
        delete [] psz_mp3_buff_raw_;
        psz_mp3_buff_raw_ = NULL;
    }

    if( ( (mpg123_handle*) state_ ) != NULL )
    {
        mpg123_close( state_ );

        state_( "mpg123_close", mpg123_close( state_ ) );
        mpg123_delete( state_ );
        state_ = (mpg123_handle *) NULL;
    }

    if( fin_.is_open() )
    {
        fin_.close();
#ifdef _DEBUG1
        logger << "closed the file" << endl;
#endif
    }
}

size_t music::read_pcm(unsigned char* prawbuffer, size_t bytes )
{
    size_t ret = 0;

    //seek loop ... must get out, what if cannot????

    if( run_ < lead_ )
    {
        logger << "covering lead of:  " << lead_.get_ms() / 1000 << " s" << endl;
    }

    while( state_ && run_ < lead_ )
    {
        specific_stream_max_writer<music> fnmaxwriter( this, &music::raw_read_pcm );
        pcm_buff_seek_.copy_max( &fnmaxwriter );

        if( run_ >= lead_   )
	{
            ret = run_.get_bytes() - lead_.get_bytes();
            pcm_buff_seek_.copyto( prawbuffer, -ret );
	}
        else
	{
            ret = 0;
	}
    }

    //logger << "run is at:  " << run_.get_ms() / 1000 << "s" << endl;

    //now do read

    size_t temp = 0;
    raw_read_pcm( prawbuffer + ret, bytes - ret, &temp );

    if( temp >= 0 )
    {
        ret += temp;
    }

    return ret;
}

int music::get_music_id()
{
    return music_id_;
}

bool music::is_onhold()
{
    return onhold_p_;
}

time_t music::get_scheduled_start()
{
    return scheduled_start_;
}

time_t music::get_scheduled_eom()
{
    return scheduled_eom_;
}

int music::get_volume_level()
{
    return volume_level_;
}

int music::get_background_volume_adjustment()
{
    return background_level_;
}

bool music::is_ready()
{
    return fin_.is_open() && !fin_.fail() && !fin_.eof();
}

bool music::is_eof()
{
    return fin_.eof();
}

void music::set_lead(milliseconds_t ms)
{
    lead_ = ms;
}

music::status_t music::get_status()
{
    return status_;
}

void music::update_status( status_t status, bool non_volatile_p  )
{
    stringstream sssql;

    status_ = status;

    if( non_volatile_p )
    {

        sssql << "update AME_PROFILE_PLAY_LIST set status = '" << statuses_[ status ] << "' where music_id = " << music_id_ << " and seq_num =  " << seq_num_ <<
            " and zone_id = " << (nchannel_ + 1);

        if( !pconn_->query().execute( sssql.str() ) )
	{
            logger << "failed to updated " << statuses_[ status ]  <<  " status channel:" << nchannel_  << " for seq " << seq_num_ << " because of " << pconn_->error();
	}

    }
}

void music::insert_play_log()
{
    stringstream sssql;

    sssql << "insert into AME_PROFILE_PLAY_LOG (CLIENT_ID, PROFILE_ID, ZONE_ID, PLAYLIST_SEQ_NUM, MEDIA_TYPE, ITEM_TYPE, DATE_STARTED, MUSIC_ID, FILE_ID, MIX_SEQ_NUM, CATEGORY_ID, VOLUME_LEVEL) select CLIENT_ID, PROFILE_ID, " << (nchannel_ + 1) << ", " << seq_num_ << ", MEDIA_TYPE, ITEM_TYPE, SCHEDULED_START, " << music_id_ << ", " << file_id_ << ", MIX_SEQ_NUM, CATEGORY_ID, VOLUME_LEVEL from AME_PROFILE_PLAY_LIST where music_id = " << music_id_ << " and seq_num =  " << seq_num_ << " and zone_id = " << (nchannel_ + 1);

    if( !pconn_->query().execute( sssql.str() ) )
    {
        logger << "failed to insert to play log channel:" << nchannel_  << " for seq " << seq_num_ << " music_id " << music_id_ << "because of " << pconn_->error();
    }
}
