#ifndef ALSA_ENGINE
#define ALSA_ENGINE

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <string>
#include <alsa/asoundlib.h>
#include "unix-result.h"
#include "alsa-result.h"
#include "ring-buffer-audio.h"
#include "logger.h"
#include "my-audio-frame.h"
#include "render.h"
//#define DEBUG0

using namespace std;

class buffer_done_t
{
public:
    buffer_done_t(){}
};

template <int NUM_CHANNELS> struct alsa_frame
                            {
                                short data[NUM_CHANNELS];
};

template <int NUM_ALSA_CHANNELS> class pcm_reader:public render<AME_AUDIO_FRAME>
{
    int nchannel_;
    alsa_frame<NUM_ALSA_CHANNELS>* pframe_buffer_;
    int num_alsa_frames_;
    int alsa_frame_;
    int media_ms_;

public:
pcm_reader():render<AME_AUDIO_FRAME>("PLAYER_ALSA"),nchannel_(0), pframe_buffer_(NULL), num_alsa_frames_(0){}
pcm_reader(int nchannel, alsa_frame<NUM_ALSA_CHANNELS>* pframe_buffer, int num_alsa_frames): render<AME_AUDIO_FRAME>("PLAYER_ALSA"), nchannel_(nchannel), pframe_buffer_(pframe_buffer), num_alsa_frames_(num_alsa_frames), alsa_frame_(0), media_ms_(0){}

    int call(AME_AUDIO_FRAME* pframes,  int frames)
    {
        return render_all_frames(pframes, frames);
    }

    uint32_t get_media_ms()
    {
        return media_ms_;
    }

    bool render_frame_specific(AME_AUDIO_FRAME* pframe)
    {
        //logger_ << "PRE_ALSA_FRAME:" << alsa_frame_ << ":channel:" << nchannel_ <<  endl;

        for( int sample = 0; sample < pframe->samples; ++sample )
        {
            pframe_buffer_[ alsa_frame_ ].data[ nchannel_ ] = pframe->raw_data[ sample ][ 0 ];
            ++alsa_frame_;
        }

        // logger_ << "POST_ALSA_PTS:"  << pframe->pts_ms << "MEDIA_MS:" << media_ms_  << ":channel:" << nchannel_ <<  endl;

        if( alsa_frame_ >= num_alsa_frames_ )
        {
            alsa_frame_ = 0;
        }

        media_ms_ += pframe->samples * 1000 / RATE;

        return true;
    }
};

template <int NUM_ALSA_CHANNELS> class alsa_engine
{
    static const unsigned int krate_ = RATE;

    string sdev_;
    snd_pcm_uframes_t frames_per_period_;
    snd_pcm_uframes_t periods_;
    unsigned int timeout_;

    snd_pcm_t* ppcm_;
    ring_buffer_audio_t* pstream_buffers_;
    logger_t logger_;
    err_logger_t err_logger_;
    specific_streamer< pcm_reader<NUM_ALSA_CHANNELS>, AME_AUDIO_FRAME >* pcm_functors_;
    alsa_frame<NUM_ALSA_CHANNELS>* pframe_buffer_;
    pcm_reader<NUM_ALSA_CHANNELS>* pcm_readers_;

    unix_result state_;

public:

    snd_pcm_uframes_t handshake( snd_pcm_uframes_t req_frames_per_period  )
    {
        alsa_result state;

        snd_pcm_hw_params_t* phwparams;
        snd_pcm_sw_params_t* pswparams;

        snd_pcm_uframes_t buffer_size = req_frames_per_period * periods_;
        frames_per_period_ = req_frames_per_period;

        logger_ << "beginning ALSA handshake:" << sdev_.c_str() << endl;

        static snd_output_t* output;
        if( !state( "snd_output_stdio", snd_output_stdio_attach(&output, stdout, 0) ) ) goto snd_pcm_nothing;

        if( !state( "snd_pcm_open", snd_pcm_open(&ppcm_, sdev_.c_str(), SND_PCM_STREAM_PLAYBACK, 0) ) ) goto snd_pcm_attached;

        logger_ << state << endl;

        if( !state( "snd_pcm_hw_params_malloc", snd_pcm_hw_params_malloc( &phwparams ) ) ) goto snd_pcm_opened;

        if( !state( "snd_pcm_hw_params_any", snd_pcm_hw_params_any( ppcm_, phwparams ) ) ) goto snd_pcm_hw_params_allocated;

        if( !state( "snd_pcm_hw_params_set_access", snd_pcm_hw_params_set_access( ppcm_, phwparams, SND_PCM_ACCESS_RW_INTERLEAVED ) ) ) goto snd_pcm_hw_params_allocated;

        if( !state( "snd_pcm_hw_params_set_format", snd_pcm_hw_params_set_format( ppcm_, phwparams, SND_PCM_FORMAT_S16_LE ) ) ) goto snd_pcm_hw_params_allocated;

        if( !state("snd_pcm_hw_params_set_rate",  snd_pcm_hw_params_set_rate( ppcm_, phwparams, krate_, 0) ) ) goto snd_pcm_hw_params_allocated;

        if( !state( "snd_pcm_hw_params_set_channels", snd_pcm_hw_params_set_channels( ppcm_, phwparams, NUM_ALSA_CHANNELS ) ) ) goto snd_pcm_hw_params_allocated;

        if( !state( "snd_pcm_hw_params_set_buffer_size_near", snd_pcm_hw_params_set_buffer_size_near( ppcm_, phwparams, &buffer_size ) ) ) goto snd_pcm_hw_params_allocated;

        if( !state( "verify buffer size", (buffer_size == req_frames_per_period * periods_) ? 0 : alsa_result::kmisc_,  "Failed to allocate buffer slices" ) )
        {
            logger_ << "instead got buffer size:" << buffer_size << endl;
        }

        if( !state( "snd_pcm_hw_params_set_period_size_near", snd_pcm_hw_params_set_period_size_near( ppcm_, phwparams, &frames_per_period_, NULL ) ) ) goto snd_pcm_hw_params_allocated;

        if( !state( "verify period",  frames_per_period_ == req_frames_per_period ? 0 : alsa_result::kmisc_,  "Failed to set interrupt period" ) )
        {
            logger_ << "instead got period size:" << frames_per_period_ << endl;
        }

        // finally apply the hardware params
        //

        if( !state( "snd_pcm_hw_params", snd_pcm_hw_params( ppcm_, phwparams ) ) ) goto snd_pcm_hw_params_allocated;

        if( !state( "snd_pcm_sw_params_malloc", snd_pcm_sw_params_malloc( &pswparams ) ) ) goto snd_pcm_hw_params_allocated;

        if( !state( "snd_pcm_sw_params_current", snd_pcm_sw_params_current( ppcm_, pswparams ) ) ) goto snd_pcm_sw_params_allocated;

        if( !state( "snd_pcm_sw_params_set_start_threshold", snd_pcm_sw_params_set_start_threshold( ppcm_, pswparams, 0 ) )) goto snd_pcm_sw_params_allocated;

        if( !state( "snd_pcm_sw_params_set_avail_min", snd_pcm_sw_params_set_avail_min( ppcm_, pswparams, frames_per_period_ ) ) ) goto snd_pcm_sw_params_allocated;

        if( !state("snd_pcm_sw_params", snd_pcm_sw_params( ppcm_, pswparams ) ) ) goto snd_pcm_sw_params_allocated;

        logger_ << "configuration:" << state << endl;

        if( pcm_functors_ ) delete [] pcm_functors_;
        if( pframe_buffer_ ) delete [] pframe_buffer_;
        if( pcm_readers_ ) delete [] pcm_readers_;

        pcm_functors_ = new specific_streamer< pcm_reader<NUM_ALSA_CHANNELS>, AME_AUDIO_FRAME >[ NUM_ALSA_CHANNELS ];

        pframe_buffer_ = new alsa_frame<NUM_ALSA_CHANNELS>[ frames_per_period_ ];

        pcm_readers_ = new pcm_reader<NUM_ALSA_CHANNELS >[NUM_ALSA_CHANNELS];

        for( int nchannel = 0; nchannel < NUM_ALSA_CHANNELS; nchannel++ )
        {
            new ( &pcm_readers_[ nchannel ] ) pcm_reader< NUM_ALSA_CHANNELS >( nchannel, pframe_buffer_, frames_per_period_  );
            new ( &pcm_functors_[ nchannel ] ) specific_streamer< pcm_reader<NUM_ALSA_CHANNELS>, AME_AUDIO_FRAME >(&pcm_readers_[ nchannel ], &pcm_reader<NUM_ALSA_CHANNELS>::call );
        }

        if( (bool) state )
        {
            logger_ << "successfully initialized ALSA using:" << " ";
            logger_ << "FRAMES_PER_PERIOD:" << frames_per_period_ << " ";
            logger_ << "buffer size:" << buffer_size << endl;
        }

        return frames_per_period_;

    snd_pcm_sw_params_allocated:  snd_pcm_sw_params_free( pswparams );
    snd_pcm_hw_params_allocated:  snd_pcm_hw_params_free( phwparams );
    snd_pcm_opened:  snd_pcm_close( ppcm_ );
    snd_pcm_attached:
    snd_pcm_nothing:  frames_per_period_ = 0;

        err_logger_ << state << endl;

        return 0;
    }

    int get_buffer_depth()
    {
        int ret;

        ret = snd_pcm_avail_update( ppcm_ );
        return ( ret  > 0 ) ? ret : 0;
    }

    int wait_period()
    {
        alsa_result ar;

        ar ( "snd_pcm_wait", snd_pcm_wait( ppcm_, timeout_) ) && ar( "snd_pcm_avail_update", snd_pcm_avail_update( ppcm_ ) );

#ifdef DEBUG0
        logger_ << ar / frames_per_period_ << " periods are there " << endl;
        logger_ << frames_per_period_ << " periods per frame " << (bool) ar << " status" << endl;
#endif

        if( (bool) !ar )
        {
            logger_ << "forced to prepare:" << ar << endl;
            ar( "snd_pcm_prepare", snd_pcm_prepare( ppcm_ ) ) && ar( "snd_pcm_avail_update", snd_pcm_avail_update( ppcm_ ) );
        }

        return ( (bool) ar) ? ar / frames_per_period_ : 0;
    }

    void read_period()
    {
        for( int nchannel = 0; nchannel < NUM_ALSA_CHANNELS; nchannel++ )
        {
            int frames_ctr = 0;
            int total_frames = (&pstream_buffers_[ nchannel ])->get_frames_per_period();

            while( frames_ctr < total_frames )
            {
                ring_buffer_audio_t* pstream_buffer =  &pstream_buffers_[ nchannel ];
                frames_ctr += pstream_buffer->read_period( &pcm_functors_[ nchannel ]  );

                if( pstream_buffer->is_done() )
                {
                    throw buffer_done_t();
                }
            }
        }
    }

    void send_period()
    {
        alsa_result ar( "snd_pcm_writei", snd_pcm_writei( ppcm_, pframe_buffer_, frames_per_period_ )  );
    }

    void cycle()
    {
        int nperiods = wait_period();

        for( int nperiod = 0; nperiod < nperiods; nperiod++ )
        {
            read_period();
            send_period();
        }
    }

alsa_engine( const char* szdev, unsigned int uperiods, unsigned int utimeout, ring_buffer_audio_t* pstream_buffers ):sdev_(szdev), frames_per_period_(0), periods_(uperiods), timeout_(utimeout), pstream_buffers_( pstream_buffers ),logger_( "PLAYER_ALSA" ), err_logger_( "PLAYER_ALSA" ), pcm_functors_(NULL), pframe_buffer_(NULL), pcm_readers_(NULL)
    {
        ppcm_ = NULL;
    }

    ~alsa_engine()
    {
        if( pcm_functors_ )
        {
            delete [] pcm_functors_;
        }

        if( pcm_readers_ )
        {
            delete [] pcm_readers_;
        }

        if( pframe_buffer_ )
        {
            delete [] pframe_buffer_;
        }
    }

    int operator()()
    {
        alsa_result ar;

        // the kernel will interrupt the inferface when 1 slice of ring buffer has played
        // and ALSA will wake up this program very soon after that.
        //
        if( ar( "snd_pcm_prepare", snd_pcm_prepare( ppcm_ )  ) )
        {
            logger_ << ar << endl;

            try
            {
                while(1)
                {
                    cycle();
                }
            }
            catch( buffer_done_t& e )
            {
                logger_ << "audio engine caught eof" << endl;
            }

            ar( "snd_pcm_close", snd_pcm_close(ppcm_) );
        }
        else
        {
            err_logger_ << "could not prepare waveform:  "  << ar << endl;
        }

        return ar;
    }

    pcm_reader<NUM_ALSA_CHANNELS>* operator[](int ndx)
    {
        return &pcm_readers_[ ndx ];
    }
};

#endif
