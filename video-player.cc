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
#include <SDL/SDL.h>

#include "env-writer.h"
#include "audio-decode-context.h"
#include "audio-silence-context.h"
#include "video-decode-context.h"
#include "render-video.h"
#include "render-audio.h"
#include "video-player.h"
#include "sdl-holder.h"

#define USER_QUIT (SDL_USEREVENT + 0)

using namespace std;

logger_t logger("VIDEO-PLAYER");

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
        video_decode_context vdc(penv->mp4_file_path, penv->ring_buffer, penv->overlay);
        if(penv->run_p) vdc();
        caux_video << "decode operation complete" << endl;
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

    SDL_Event event;
    event.type = MY_DONE;
    SDL_PushEvent(&event);

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

        sleep(100);

        audio_decode_context adc(penv->mp4_file_path, penv->ring_buffer);
        if(penv->run_p) adc();
        logger << "decode operation complete" << endl;
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

    SDL_Event event;
    event.type = MY_DONE;
    if(penv->run_p) SDL_PushEvent(&event);

    return &penv->ret;
}

static void* audio_silence_context_thread(void *parg)
{
    env_audio_silence_context* penv = (env_audio_silence_context*) parg;

    try
    {
        logger_t logger("ZONE-PLAYER");

        if( !penv->buffer_ready->wait() )
        {
            throw app_fault("premature end of audio silence");
        }

        sleep(100);

        audio_silence_context asc(penv->ring_buffer);
        if(penv->run_p) asc();
        logger << "silence operation complete" << endl;
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

    SDL_Event event;
    event.type = MY_DONE;
    if(penv->run_p) SDL_PushEvent(&event);

    return &penv->ret;
}

static void* render_video_thread(void *parg)
{
    env_render_video_context* penv = (env_render_video_context*) parg;

    try
    {
        render_video rv(penv->ring_buffer, penv->overlay);

        penv->video_ready->signal(true);
        if( !penv->audio_ready->wait() )
        {
            throw app_fault("premature end of render video");
        }

        if(penv->run_p) rv();

        caux_video << "display complete" << endl;
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

    SDL_Event event;
    event.type = MY_DONE;
    SDL_PushEvent(&event);

    return &penv->ret;
}

static void* render_audio_thread(void *parg)
{
    env_render_audio_context* penv = (env_render_audio_context*) parg;

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
                new ( &penv->ring_buffers[ channel ] ) ring_buffer_audio_t ( penv->frames_per_period / AUDIO_PACKET_SIZE, penv->periods_mpeg );
            }

            penv->buffer_ready->broadcast(true);
        }
        else
        {
            throw app_fault( "could not do ALSA handshake" );
        }

        penv->audio_ready->signal(true);
        if( !penv->video_ready->wait() )
        {
            throw app_fault( "premature end of render audio" );
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

    SDL_Event event;
    event.type = MY_DONE;
    if(penv->run_p) SDL_PushEvent(&event);

    return &penv->ret;
}

int run_play(const char* mp4_file_path)
{
    int VIDEO_WIDTH = getenv_numeric( "VIDEO_WIDTH", 854);
    int VIDEO_HEIGHT = getenv_numeric( "VIDEO_HEIGHT", 480);

    string ALSA_DEV = getenv_string( "ALSA_DEV", "hw:0,0" );
    int NUM_CHANNELS = getenv_numeric( "NUM_CHANNELS", 4 );
    int PERIODS_ALSA = getenv_numeric( "PERIODS_ALSA", 8 );
    int PERIODS_MPEG = getenv_numeric( "PERIODS_MPEG", 128 );
    int FRAMES_PER_PERIOD = getenv_numeric( "FRAMES_PER_PERIOD", 1024 );
    int WAIT_TIMEOUT = getenv_numeric( "WAIT_TIMEOUT", 2000 );

    ring_buffer_video_t ring_buffer_video(24,  6);
    env_video_decode_context env_vdc;
    env_render_video_context env_render_video;
    env_audio_decode_context env_adc;
    env_audio_silence_context env_asc[3];
    env_render_audio_context env_render_audio;

    synch_t< bool >  buffer_ready(false);
    synch_t< bool > audio_ready;
    synch_t< bool > video_ready;

    int ret = 0;
    pthread_t thread_vfc, thread_afc, thread_asc[3], thread_render_video, thread_render_audio;
    list<pthread_t*> threads;

    threads.push_back(&thread_vfc);
    threads.push_back(&thread_afc);

    for(int idx=0; idx < 3; ++idx)
    {
        threads.push_back(&thread_asc[idx]);
    }

    threads.push_back(&thread_render_video);
    threads.push_back(&thread_render_audio);

    try
    {
        sdl_holder sdl(VIDEO_WIDTH, VIDEO_HEIGHT, NUM_CHANNELS, threads);

        //set up the video decode
        //

        env_vdc.mp4_file_path = mp4_file_path;
        env_vdc.overlay = sdl.get_overlay();
        env_vdc.start_at = 0;
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

        //set up the audio decode
        //

        ring_buffer_audio_t pcm_buffers[ 4  ];

        env_adc.mp4_file_path = mp4_file_path;
        env_adc.channel = 0;
        env_adc.start_at = 0;
        env_adc.ring_buffer = &pcm_buffers[ 0 ];
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

        // set up the silence threads
        //

        for( int idx = 0; idx < 3; ++idx )
        {
            int channel = idx + 1;
            env_asc[idx].channel = channel;
            env_asc[idx].ring_buffer = &pcm_buffers[ channel ];
            env_asc[idx].buffer_ready = &buffer_ready;
            env_asc[idx].run_p = true;
            env_asc[idx].ret = 0;

            ret = pthread_create( &thread_asc[idx], NULL, &audio_silence_context_thread, &env_asc[idx] );

            if( ret < 0 )
            {
                stringstream ss;
                ss << "audio silence thread create error:"  << strerror( ret );
                throw app_fault( ss.str().c_str() );
            }
        }

        env_render_video.ring_buffer = &ring_buffer_video;
        env_render_video.overlay = sdl.get_overlay();
        env_render_video.audio_ready = &audio_ready;
        env_render_video.video_ready = &video_ready;
        env_render_video.run_p = true;

        //set up the render video
        //

        ret = pthread_create( &thread_render_video, NULL, &render_video_thread, &env_render_video );

        if( ret < 0 )
        {
            stringstream ss;
            ss << "render video thread create error:"  << strerror( ret );
            throw app_fault( ss.str().c_str() );
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
        env_render_audio.buffer_ready = &buffer_ready;
        env_render_audio.audio_ready = &audio_ready;
        env_render_audio.video_ready = &video_ready;
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
        sdl.message_loop(&ring_buffer_video, &pcm_buffers[0], &buffer_ready, &video_ready, &audio_ready);
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

int main(int argc, char *argv[])
{
    av_register_all();

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

    int ret = run_play( "/mnt/MUSIC-THD/test.hd.mp4" );

    av_lockmgr_register(NULL);

    SDL_Quit();

    sdl_holder::logger << "final bye" << endl;

    return ret;
}
