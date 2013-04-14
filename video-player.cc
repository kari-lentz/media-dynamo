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
#include "video-decode-context.h"
#include "display.h"
#include "video-player.h"
#include "sdl-holder.h"

#define USER_QUIT (SDL_USEREVENT + 0)

using namespace std;

static void* video_decode_context_thread(void *parg)
{
    env_video_decode_context* penv = (env_video_decode_context*) parg;

    try
    {
        video_decode_context vdc(penv->mp4_file_path, penv->ring_buffer, penv->overlay);
        vdc();
        caux_video << "decode operation complete" << endl;
        penv->ret = 0;

        SDL_Event event;
        event.type = MY_DONE;
        SDL_PushEvent(&event);
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

    return &penv->ret;
}

static void* display_thread(void *parg)
{
    env_display_context* penv = (env_display_context*) parg;

    try
    {
        display d(penv->ring_buffer, penv->overlay);
        d();
        caux_video << "display complete" << endl;
        penv->ret = 0;

        SDL_Event event;
        event.type = MY_DONE;
        SDL_PushEvent(&event);
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

    return &penv->ret;
}

int run_decode(const char* mp4_file_path)
{
    ring_buffer_t ring_buffer(24,  6);
    env_video_decode_context env_vdc;
    env_display_context env_display;
    int ret = 0;
    pthread_t thread_fc, thread_display;

    try
    {
        sdl_holder sdl(854, 480, &thread_fc, &thread_display);

        env_vdc.mp4_file_path = mp4_file_path;
        env_vdc.overlay = sdl.get_overlay();
        env_vdc.start_at = 0;
        env_vdc.ring_buffer = &ring_buffer;
        env_vdc.ret = 0;

        int ret = pthread_create( &thread_fc, NULL, &video_decode_context_thread, &env_vdc );

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

        sdl.message_loop(ring_buffer);
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

  int ret = run_decode( "/mnt/MUSIC-THD/test.hd.mp4" );

  SDL_Quit();

  sdl_holder::logger << "final bye" << endl;

  return ret;
}
