using namespace std;

#include "render-video.h"

template <> logger_t render<AME_VIDEO_FRAME>::logger("VIDEO-PLAYER");

bool render_video::render_frame_specific(AME_VIDEO_FRAME* pframe)
{
    SDL_LockYUVOverlay( overlay_ );

    memcpy(overlay_->pixels[ 0 ], pframe->data[ 0 ], overlay_->pitches[0] * overlay_->h);
    memcpy(overlay_->pixels[ 2 ], pframe->data[ 1 ], (overlay_->pitches[2] * overlay_->h) >> 1);
    memcpy(overlay_->pixels[ 1 ], pframe->data[ 2 ], (overlay_->pitches[1] * overlay_->h) >> 1);

    SDL_UnlockYUVOverlay( overlay_ );

    SDL_Rect rect;

    rect.x = 0;
    rect.y = 0;
    rect.w = overlay_->w;
    rect.h = overlay_->h;

    int ret = SDL_DisplayYUVOverlay(overlay_, &rect);

    if( ret != 0 )
    {
        stringstream ss;
        ss << "SDL: could not display YUV overlay";
        throw app_fault( ss.str().c_str() );
    }

    return ret == 0;
}

render_video::render_video(ring_buffer_video_t* pbuffer, SDL_Overlay* overlay):render(), pbuffer_(pbuffer), overlay_(overlay), functor_(this, &render_video::render_frames)
{
}

render_video::~render_video()
{
}

int render_video::operator()()
{
    do
    {
        pbuffer_->read_avail( &functor_ );

        int delay = 20;
        usleep( delay * 1000 );

    }  while( !pbuffer_->is_done() );

    return 0;
}
