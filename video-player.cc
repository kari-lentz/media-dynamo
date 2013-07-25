// Maximum number of bytes allowed to be read from stdin
static const unsigned long STDIN_MAX = 1000000;

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
#include <sstream>
#include <map>
#include <mysql++/mysql++.h>
#include <mpg123.h>
#include <SDL/SDL.h>

#include "messages.h"
#include "env-writer.h"
#include "audio-decode-context.h"
#include "mp3-decode-context.h"
#include "video-decode-context.h"
#include "render-audio.h"
#include "video-player.h"
#include "sdl-holder.h"
#include "cairo-f.h"
#include "dom-context.h"
#include "decoder-result.h"
#include "decoder-inst.h"

#define USER_QUIT (SDL_USEREVENT + 0)

using namespace std;

logger_t logger("VIDEO-PLAYER");
bool g_fullscreen_p = false;
int g_start_at = 0;

int getenv_numeric( const char* szname, int default_val )
{
    bool numeric_p = false;
    char* szvalue = getenv( szname );

    if( szvalue && szvalue[0] != '\0' )
    {
        numeric_p = true;

        for(int i = 0; szvalue[ i ] != '\0'; i++ )
	{
            if( !(szvalue[ i ] >= '0' && szvalue[i] <= '9') )
	    {
                numeric_p = false;
                break;
	    }
	}
    }

    return numeric_p ? atoi( szvalue ):default_val;
}

string getenv_string( const char* szname, const char* szdefault )
{
    char* szvalue = getenv( szname );

    return ( szvalue && szvalue[0] != '\0' )  ? szvalue : szdefault;
}

static int lockmgr(void **arg, enum AVLockOp op)
{
    int ret;

   switch(op)
   {
   case AV_LOCK_CREATE:
   {
       pthread_mutex_t* pmtx = new pthread_mutex_t;
       *arg = pmtx;
       if( pthread_mutex_init(pmtx, NULL) != 0 )
           ret=1;
       else
           ret=0;
       break;
   }
   case AV_LOCK_OBTAIN:
   {
       pthread_mutex_t* pmtx = (pthread_mutex_t*) *arg;
       ret = !!pthread_mutex_lock(pmtx);
       break;
   }
   case AV_LOCK_RELEASE:
   {
       pthread_mutex_t* pmtx = (pthread_mutex_t*) *arg;
       ret =  !!pthread_mutex_unlock(pmtx);
       break;
   }
   case AV_LOCK_DESTROY:
   {
       pthread_mutex_t* pmtx = (pthread_mutex_t*) *arg;
       pthread_mutex_destroy(pmtx);
       ret = 0;
       break;
   }
   default:
   {
       ret = 1;
       break;
   }
   }

   return ret;
}

static void* video_decode_context_thread(void *parg)
{
    env_video_decode_context* penv = (env_video_decode_context*) parg;

    try
    {
        video_decode_context vdc(penv->mp4_file_path, penv->ring_buffer);
        if(penv->run_p) vdc(penv->start_at);
        penv->ret = 0;
    }
    catch( app_fault& e )
    {
        penv->ret = -1;
        decode_context<AME_VIDEO_FRAME>::logger << "caught exception:" << e << endl;
    }
    catch (const std::ios_base::failure& e)
    {
        penv->ret = -1;
        decode_context<AME_VIDEO_FRAME>::logger << "Exception opening/reading file" << endl;
    }
    catch (exception& e)
    {
        penv->ret = -1;
        decode_context<AME_VIDEO_FRAME>::logger << "Exception opening/reading file" << endl;
    }

    post_message( MY_DONE );
    caux_video << "video decode operation complete" << endl;

    return &penv->ret;
}

static void* audio_decode_context_thread(void *parg)
{
    env_audio_decode_context* penv = (env_audio_decode_context*) parg;

    try
    {
        logger_t logger("ZONE-PLAYER");
        if( !penv->buffer_ready->wait() )
        {
            throw app_fault("premature end of audio decode");
        }

        logger << "AUDIO BUFFER READY:" << penv->channel << endl;

        audio_decode_context adc(penv->mp4_file_path, penv->ring_buffer);
        if(penv->run_p) adc(penv->start_at);
        penv->ret = 0;
    }
    catch( app_fault& e )
    {
        penv->ret = -1;
        logger << "caught exception:" << e << endl;
    }
    catch (const std::ios_base::failure& e)
    {
        penv->ret = -1;
        logger << "Exception opening/reading file" << endl;
    }
    catch (exception& e)
    {
        penv->ret = -1;
        logger << "Exception opening/reading file" << endl;
    }

    if(penv->run_p) post_message( MY_DONE);
    logger << "audio decode operation complete" << endl;

    return &penv->ret;
}

static void* mp3_decode_context_thread(void *parg)
{
    env_mp3_decode_context* penv = (env_mp3_decode_context*) parg;
    mysqlpp::Connection conn;
    penv->conn = &conn;

    try
    {
        logger_t logger("ZONE-PLAYER");

        if( !penv->buffer_ready->wait() )
        {
            throw app_fault("premature end of audio silence");
        }

        decoder_inst mp3_context(penv);

        if(penv->run_p) mp3_context();
        penv->ret = 0;
    }
    catch( app_fault& e )
    {
        penv->ret = -1;
        logger << "caught exception:" << e << endl;
    }
    catch (const std::ios_base::failure& e)
    {
        penv->ret = -1;
        logger << "Exception opening/reading file" << endl;
    }
    catch (exception& e)
    {
        penv->ret = -1;
        logger << "Exception opening/reading file" << endl;
    }

    if(penv->run_p) post_message(MY_DONE);
    logger << "silence operation complete" << endl;

    return &penv->ret;
}

static void* render_audio_thread(void *parg)
{
    env_render_audio_context* penv = (env_render_audio_context*) parg;

    int primed_message[ 4 ] = { MY_AUDIO_PRIMED_0, MY_AUDIO_PRIMED_1, MY_AUDIO_PRIMED_2, MY_AUDIO_PRIMED_3};

    try
    {
        //run place constructor over ring buffer
        //
        alsa_engine<4> ae(penv->alsa_dev.c_str(), penv->periods_alsa, penv->wait_timeout, penv->ring_buffers);
        penv->frames_per_period = ae.handshake( penv->frames_per_period );
        if( penv->frames_per_period  > 0 )
        {
            for( int channel = 0; channel < penv->num_channels; channel++ )
            {
                new ( &penv->ring_buffers[ channel ] ) ring_buffer_audio_t ( penv->frames_per_period / AUDIO_PACKET_SIZE, penv->periods_mpeg, primed_message[ channel ] );
            }

            post_message( MY_AUDIO_BUFFERS_READY );
        }
        else
        {
            throw app_fault( "could not do ALSA handshake" );
        }

        if( !penv->media_ready->wait() )
        {
            throw app_fault("premature end of waiting for audio to get ready");
        }
        else
        {
            decode_context<AME_VIDEO_FRAME>::logger << "AUDIO now ready and primed" << endl;
        }

        if( penv->run_p ) penv->ret = ae();
    }
    catch( app_fault& e )
    {
        penv->ret = -1;
        decode_context<AME_VIDEO_FRAME>::logger << "caught exception:" << e << endl;
    }
    catch (const std::ios_base::failure& e)
    {
        penv->ret = -1;
        decode_context<AME_VIDEO_FRAME>::logger << "Exception opening/reading file" << endl;
    }
    catch (exception& e)
    {
        penv->ret = -1;
        decode_context<AME_VIDEO_FRAME>::logger << "Exception opening/reading file" << endl;
    }

    if(penv->run_p) post_message(MY_DONE);
    logger << "render audio operation complete" << endl;

    return &penv->ret;
}

int run_play(const char* mp4_file_path, int start_at, bool fullscreen_p)
{
    int VIDEO_WIDTH = getenv_numeric( "VIDEO_WIDTH", MAX_SCREEN_WIDTH);
    int VIDEO_HEIGHT = getenv_numeric( "VIDEO_HEIGHT", MAX_SCREEN_HEIGHT);

    string ALSA_DEV = getenv_string( "ALSA_DEV", "hw:0,0" );
    int NUM_CHANNELS = getenv_numeric( "NUM_CHANNELS", 4 );
    int PERIODS_ALSA = getenv_numeric( "PERIODS_ALSA", 8 );
    int PERIODS_MPEG = getenv_numeric( "PERIODS_MPEG", 128 );
    int FRAMES_PER_PERIOD = getenv_numeric( "FRAMES_PER_PERIOD", 1024 );
    int WAIT_TIMEOUT = getenv_numeric( "WAIT_TIMEOUT", 2000 );

    ring_buffer_video_t ring_buffer_video(24,  6, MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT);
    ring_buffer_audio_t pcm_buffers[ 4  ];

    env_video_decode_context env_vdc;
    env_audio_decode_context env_adc;
    env_mp3_decode_context env_mp3[3];
    env_render_audio_context env_render_audio;

    synch_t< bool >  buffer_ready(false);
    synch_t< bool > media_ready(false);

    int ret = 0;
    pthread_t thread_vfc, thread_afc, thread_mp3[3], thread_render_audio;
    list<pthread_t*> threads;

    logger << "welcome to media dynamo" << endl;

    // set up the silence threads
    //
    int silence_channels [] = {0, 2, 3};
    int audio_channel = 1;

    threads.push_back(&thread_vfc);
    threads.push_back(&thread_afc);

    for(int idx=0; idx < 3; ++idx)
    {
        threads.push_back(&thread_mp3[idx]);
    }

    threads.push_back(&thread_render_audio);

    try
    {
        sdl_holder sdl(&ring_buffer_video, &pcm_buffers[0], NUM_CHANNELS, threads, fullscreen_p);

        //set up the video decode
        //

        env_vdc.mp4_file_path = mp4_file_path;
        env_vdc.start_at = start_at * 1000;
        env_vdc.ring_buffer = &ring_buffer_video;
        env_vdc.run_p = true;
        env_vdc.ret = 0;

        ret = pthread_create( &thread_vfc, NULL, &video_decode_context_thread, &env_vdc );

        if( ret < 0 )
        {
            stringstream ss;
            ss << "video file thread create error:"  << strerror( ret );
            throw app_fault( ss.str().c_str() );
        }

        env_adc.mp4_file_path = mp4_file_path;
        env_adc.channel = audio_channel;
        env_adc.start_at = start_at * 1000;
        env_adc.ring_buffer = &pcm_buffers[ audio_channel ];
        env_adc.buffer_ready = &buffer_ready;
        env_adc.run_p = true;
        env_adc.ret = 0;

        ret = pthread_create( &thread_afc, NULL, &audio_decode_context_thread, &env_adc );

        if( ret < 0 )
        {
            stringstream ss;
            ss << "audio file thread create error:"  << strerror( ret );
            throw app_fault( ss.str().c_str() );
        }

        for( int idx = 0; idx < 3; ++idx )
        {
            int channel = silence_channels[ idx ];
            env_mp3[idx].channel = channel;
            env_mp3[idx].ring_buffer = &pcm_buffers[ channel ];
            env_mp3[idx].buffer_ready = &buffer_ready;
            env_mp3[idx].run_p = true;
            env_mp3[idx].ret = 0;

            ret = pthread_create( &thread_mp3[idx], NULL, &mp3_decode_context_thread, &env_mp3[idx] );

            if( ret < 0 )
            {
                stringstream ss;
                ss << "audio silence thread create error:"  << strerror( ret );
                throw app_fault( ss.str().c_str() );
            }
        }


        //set up the audio rendering
        //

        env_render_audio.ring_buffers = &pcm_buffers[0];
        env_render_audio.alsa_dev = ALSA_DEV;
        env_render_audio.num_channels = NUM_CHANNELS;
        env_render_audio.periods_alsa = PERIODS_ALSA;
        env_render_audio.wait_timeout = WAIT_TIMEOUT;
        env_render_audio.frames_per_period = FRAMES_PER_PERIOD;
        env_render_audio.periods_mpeg = PERIODS_MPEG;
        env_render_audio.media_ready = &media_ready;
        env_render_audio.run_p = true;
        env_render_audio.debug_p = false;

        ret = pthread_create( &thread_render_audio, NULL, &render_audio_thread, &env_render_audio );

        if( ret < 0 )
        {
            stringstream ss;
            ss << "render audio thread create error:"  << strerror( ret );
            throw app_fault( ss.str().c_str() );
        }

        //main SDL loop for everything
        //
        sdl.message_loop(&buffer_ready, &media_ready);
    }
    catch(app_fault& e)
    {
        sdl_holder::logger << "caught exception:" << e << endl;
        ret = -1;
    }
    catch (const std::ios_base::failure& e)
    {
        logger << "Exception opening/reading file";
        ret = -1;
    }
    catch (exception& e)
    {
        logger << "Exception opening/reading file";
        ret = -1;
    }

    return ret;
}

string process_param(const char* szparam)
{
    string param = szparam;

    int len = param.length();
    bool prefix_p = true;
    string ret;

    for( int idx = 0; idx < len; ++idx )
    {
        char c = param[ idx ];

        if(prefix_p)
        {
            if(c != '-')
            {
                prefix_p = false;
            }
        }

        if( !prefix_p )
        {
            if( c >= 'A' && c <= 'Z' )
            {
                ret.push_back( c  + 33 );
            }
            else
            {
                ret.push_back(c);
            }
        }
    }

    return ret;
}

void parse_cmdline(int argc, char *argv[])
{
    string param;
    string prev_param;

    for(int idx = 1; idx < argc; ++idx)
    {
        param = process_param( argv[ idx ] );

        if( param == "fullscreen")
        {
            g_fullscreen_p = true;
        }
        else if( prev_param == "start-at" )
        {
            g_start_at = atoi(param.c_str());
        }

        prev_param = param;
    }
}

int main(int argc, char *argv[])
{
    av_register_all();
    dom_context_t::register_all();
    decoder_result state ( decoder_result::ALL_CHANNEL,  "mpg123_init", mpg123_init() );

    parse_cmdline(argc, argv);

    if (av_lockmgr_register(lockmgr))
    {
        logger << "Could not initialize lock manager!" << endl;
        return -1;
    }

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER))
    {
        logger << "Could not initialize SDL - " << SDL_GetError();
        return -1;
    }

    int ret = run_play( "/mnt/MUSIC-THD/test.hd.mp4", g_start_at, g_fullscreen_p );

    av_lockmgr_register(NULL);

    SDL_Quit();
    mpg123_exit();

    sdl_holder::logger << "final bye" << endl;

    return ret;
}
