#include "smart-ptr.h"
#include "result.h"
#include "decoder-result.h"
#include "ring-buffer-audio.h"
#include <openssl/rc4.h>
#include <fstream>
#include <queue>
#include <mysql++/mysql++.h>
#include "logger.h"
#include "mp3-decode-context.h"

#ifndef MUSIC_H
#define MUSIC_H

#define LEAD_TIME_EOM (10)
#define QUEUE_LEAD_TIME_MIN (3600)
#define QUEUE_LEAD_TIME_MAX (14400)

typedef int milliseconds_t;
typedef int bytes_decoded_t;
typedef int samples_t;

typedef packed_array< char > pa_char;
typedef ptr< pa_char > ppa_char;

class run_t
{
    samples_t samples_;

public:

run_t():samples_(0){}
run_t( milliseconds_t ms ):samples_( ( ( (double) ms) / 1000.0 ) * DFRATE ){}

    run_t& operator = (milliseconds_t ms)
                      {
                          samples_ = ( ( (double) ms) / 1000.0 ) * DFRATE;
                      }

    void inc_bytes(bytes_decoded_t bytes)
    {
        samples_ += bytes * 8 / BITS_PER_SAMPLE;
    }

    milliseconds_t get_ms()
    {
        return ( (double) samples_) / DFRATE * 1000.0;
    }

    samples_t get_samples()
    {
        return samples_;
    }

    bytes_decoded_t get_bytes()
    {
        return samples_ * BITS_PER_SAMPLE / 8;
    }

    bool operator > (run_t& run )
    {
        return (samples_ > run.samples_);
    }

    bool operator >= (run_t& run )
    {
        return (samples_ >= run.samples_);
    }

    bool operator < (run_t& run )
    {
        return (samples_ < run.samples_);
    }

    bool operator <= (run_t& run )
    {
        return (samples_ <= run.samples_);
    }
};

class zone_env_t
{
public:

    // zone specific environent variables
    mysqlpp::Connection conn_;
    int nchannel_;
    ring_buffer_audio_t* ppcm_buffer_;
    logger_t logger_;
    err_logger_t err_logger_;
    warning_logger_t warning_logger_;

zone_env_t():logger_( "ZONE_PLAYER", nchannel_ ), err_logger_( "ZONE_PLAYER", nchannel_ ), warning_logger_( "ZONE_PLAYER", nchannel_ ), conn_( false )
    {
        nchannel_ = 0;
        ppcm_buffer_ = NULL;
    }

zone_env_t( int nchannel, ring_buffer_audio_t* ppcm_buffer ):logger_( "ZONE_PLAYER", nchannel + 1 ), err_logger_( "ZONE_PLAYER", nchannel + 1 ), warning_logger_( "ZONE_PLAYER", nchannel + 1 ), conn_( false )
    {
        // zone specific environent variables
        nchannel_ = nchannel;
        ppcm_buffer_ = ppcm_buffer;
    }
};

class music:public ref_ctr
{
public:
    enum status_t {eNULL = 0, eSkipped = 1, eQueued = 2, ePrimed = 3, ePlaying = 4, ePlayed = 5, eNumStatuses = 6};

private:

    static const char* statuses_[];

    int music_id_;
    bool onhold_p_;
    int file_id_;
    int volume_level_;
    int background_level_;
    int seq_num_;
    time_t scheduled_start_;
    time_t scheduled_eom_;

    //playlist filler

    //

    run_t run_;
    run_t lead_;
    run_t real_eom_;
    status_t status_;

    RC4_KEY key_;

    decoder_result state_;
    bool bneed_data_;
    int nchannel_;

    ifstream fin_;
    unsigned char* psz_mp3_buff_raw_;
    packed_array<unsigned char> mp3_buff_whole_;
    packed_array<unsigned char> mp3_buff_remainder_;
    packed_array<unsigned char> pcm_buff_seek_;

    ptr< packed_array<char> > get_file_path( int music_id );

    // zone environment variables
    mysqlpp::Connection* pconn_;

    void debug_read(packed_array<unsigned char>& buffer);
    decoder_result& open_file();

    int read_mp3( unsigned char* pbuffer, int nbytes );

    decoder_result& feed_mp3();
    int raw_read_pcm(void* prawbuffer, size_t bytes, size_t* pbytes_read);

    ppa_char calculate_file_path();

    static logger_t logger;

public:

    music(env_mp3_decode_context* pzone, int music_id, bool onhold, int seq_num, time_t scheduled_start, time_t scheduled_eom, int volume_level, int background_level );

    ~music();

    size_t read_pcm(unsigned char* prawbuffer, size_t bytes);

    int get_music_id();
    bool is_onhold();

    time_t get_scheduled_start();
    time_t get_scheduled_eom();

    int get_volume_level();
    int get_background_volume_adjustment();

    status_t get_status();
    void set_lead( milliseconds_t ms );

    bool is_ready();
    bool is_eof();

    void update_status( status_t status, bool non_volatipe_p = true  );
    void insert_play_log();
};

typedef ptr< music > pmusic_t;

#endif


