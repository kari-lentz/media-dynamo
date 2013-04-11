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

using namespace std;

void initialize_SDL(int width, int height, SDL_Surface** ppsurface, SDL_Overlay** ppoverlay)
{
    *ppsurface = SDL_SetVideoMode(width, height, 0, 0);
    if(!*ppsurface)
    {
        stringstream ss;
        ss << "SDL: could not set video mode - exiting";
        throw app_fault( ss.str().c_str() );
    }

    *ppoverlay = SDL_CreateYUVOverlay(width, height, SDL_YV12_OVERLAY, *ppsurface);

    if( !ppoverlay )
    {
        stringstream ss;
        ss << "SDL: could not create YUV overlay";
        throw app_fault( ss.str().c_str() );
    }
}

static void* video_file_context_thread(void *parg)
{
    env_file_context* penv = (env_file_context*) parg;

    try
    {
        video_file_context vfc(penv->mp4_file_path, penv->ring_buffer, penv->overlay);
        vfc();
        caux << "encode operation complete" << endl;
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

int run_decode(const char* mp4_file_path)
{
    ring_buffer_t ring_buffer(24,  6);
    env_file_context env;
    int ret;
    pthread_t thread_fc;

    env.mp4_file_path = mp4_file_path;
    env.start_at = 0;
    env.ring_buffer = &ring_buffer;
    env.ret = 0;

    try
    {
        SDL_Surface* surface;
        initialize_SDL(854, 480, &surface, &env.overlay);
    }
    catch(app_fault& e)
    {
        display::logger << "caught exception:" << e << endl;
        return -1;
    }

    try
    {
        int ret = pthread_create( &thread_fc, NULL, &video_file_context_thread, &env );

        if( ret < 0 )
        {
            stringstream ss;
            ss << "writer thread create error:"  << strerror( ret );
            throw app_fault( ss.str().c_str() );
        }

        display d(&ring_buffer, env.overlay);
        ret = d();

        display::logger << "will wait for encode thread completion:last ret:" << ret << endl;
    }
    catch(app_fault& e)
    {
        display::logger << "caught exception:" << e << endl;
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

    pthread_join( thread_fc, NULL );
    display::logger << "request was processed" << endl;

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

  return run_decode( "/mnt/MUSIC/test.hd.mp4" );
}
