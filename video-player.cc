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
#include "video-file-context.h"
#include "display.h"
#include "video-player.h"
#include "sdl-holder.h"

#define USER_QUIT (SDL_USEREVENT + 0)

using namespace std;

logger_t logger("VIDEO-DISPLAY");

static void* video_file_context_thread(void *parg)
{
    env_video_file_context* penv = (env_video_file_context*) parg;

    try
    {
        video_file_context vfc(penv->mp4_file_path, penv->ring_buffer, penv->overlay);
        vfc();
        caux << "decode operation complete" << endl;
        penv->ret = 0;
    }
    catch( app_fault& e )
    {
        penv->ret = -1;
        decode_context::logger << "caught exception:" << e << endl;
    }
    catch (const std::ios_base::failure& e)
    {
        penv->ret = -1;
        decode_context::logger << "Exception opening/reading file" << endl;
    }
    catch (exception& e)
    {
        penv->ret = -1;
        decode_context::logger << "Exception opening/reading file" << endl;
    }

    return &penv->ret;
}

static void* display_thread(void *parg)
{
    env_display_context* penv = (env_display_context*) parg;

    try
    {
        display d(penv->ring_buffer, penv->overlay);
        d();
        caux << "display complete" << endl;
        penv->ret = 0;
    }
    catch( app_fault& e )
    {
        penv->ret = -1;
        decode_context::logger << "caught exception:" << e << endl;
    }
    catch (const std::ios_base::failure& e)
    {
        penv->ret = -1;
        decode_context::logger << "Exception opening/reading file" << endl;
    }
    catch (exception& e)
    {
        penv->ret = -1;
        decode_context::logger << "Exception opening/reading file" << endl;
    }

    return &penv->ret;
}

void do_exit(pthread_t thread_display, pthread_t thread_fc)
{
    sdl_holder::done = (lock_t<bool>) true;

    pthread_join( thread_display, NULL );
    pthread_join( thread_fc, NULL );
    display::logger << "all secondary threads down" << endl;
}

int run_decode(const char* mp4_file_path)
{
    ring_buffer_t ring_buffer(24,  6);
    env_video_file_context env_fc;
    env_display_context env_display;
    int ret = 0;
    pthread_t thread_fc, thread_display;

    try
    {
        sdl_holder sdl(854, 480, &thread_fc, &thread_display);

        env_fc.mp4_file_path = mp4_file_path;
        env_fc.overlay = sdl.get_overlay();
        env_fc.start_at = 0;
        env_fc.ring_buffer = &ring_buffer;
        env_fc.ret = 0;

        int ret = pthread_create( &thread_fc, NULL, &video_file_context_thread, &env_fc );

        if( ret < 0 )
        {
            stringstream ss;
            ss << "file thread create error:"  << strerror( ret );
            throw app_fault( ss.str().c_str() );
        }

        env_display.ring_buffer = &ring_buffer;
        env_display.overlay = sdl.get_overlay();

        ret = pthread_create( &thread_display, NULL, &display_thread, &env_display );

        if( ret < 0 )
        {
            stringstream ss;
            ss << "display thread create error:"  << strerror( ret );
            throw app_fault( ss.str().c_str() );
        }

        sdl.message_loop();
    }
    catch(app_fault& e)
    {
        sdl_holder::logger << "caught exception:" << e << endl;
        ret = -1;
    }
    catch (const std::ios_base::failure& e)
    {
        display::logger << "Exception opening/reading file";
        ret = -1;
    }
    catch (exception& e)
    {
        display::logger << "Exception opening/reading file";
        ret = -1;
    }

    return ret;
}

int main(int argc, char *argv[])
{
   av_register_all();

  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
  {
      fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
      exit(1);
  }

  int ret = run_decode( "/mnt/MUSIC/test.hd.mp4" );

  SDL_Quit();

  sdl_holder::logger << "final bye" << endl;

  return ret;
}
