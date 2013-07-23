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
#include <map>
#include <string>
#include <sstream>
#include <SDL/SDL.h>

#include "messages.h"
#include "video-decode-context.h"
#include "sdl-holder.h"
#include "asset.h"

using namespace std;

template <> logger_t decode_context<AME_VIDEO_FRAME>::logger("VIDEO-PLAYER");

void video_decode_context::buffer_primed()
{
    post_message( MY_VIDEO_PRIMED );
}

void video_decode_context::run_commands(cairo_t* cr, int media_ms)
{
    for( list<asset_t*>::iterator it = assets_.begin(); it != assets_.end(); ++it )
    {
        if( (*it)->is_visible(media_ms) )
        {
            (*it)->render(cr);
        }
    }
}

void video_decode_context::with_cairo(AME_VIDEO_FRAME* frame)
{
    cairo_surface_t* cairo_surface_assets;
    cairo_t* cr_assets;

    cairo_surface_assets = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32,
        frame->width,
        frame->height);

    if( !cairo_surface_assets )
    {
        stringstream ss;
        ss << "could not create surface" << endl;
        throw app_fault( "" );
    }

    cr_assets = cairo_create (cairo_surface_assets);
    if( !cr_assets )
    {
        stringstream ss;
        ss << "could not create assets surface" << endl;
        throw app_fault( "" );
    }

    run_commands(cr_assets, frame->pts_ms);

    cairo_surface_t* cairo_surface_video;
    cairo_t* cr_video;

    int stride = cairo_format_stride_for_width( CAIRO_FORMAT_ARGB32, frame->width );

/* ... make sure sdlsurf is locked or doesn't need locking ... */
    cairo_surface_video = cairo_image_surface_create_for_data (
        (unsigned char*) frame->raw_data,
        CAIRO_FORMAT_ARGB32,
        frame->width,
        frame->height,
        stride);

    cr_video = cairo_create (cairo_surface_video);
    if( !cr_video )
    {
        stringstream ss;
        ss << "could not create video surface" << endl;
        throw app_fault( "" );
    }

    cairo_set_source_surface(cr_video, cairo_surface_assets, 0, 0);

    cairo_pattern_t* nothing = cairo_pattern_create_rgba(0, 0, 0, 1.0);
    cairo_mask (cr_video, nothing);
    cairo_fill(cr_video);

    // destroy assets
    //

    if( cr_assets )
    {
        cairo_destroy( cr_assets );
        cr_assets = NULL;
    }

    if( cairo_surface_assets != NULL)
    {
        cairo_surface_destroy( cairo_surface_assets );
        cairo_surface_assets = NULL;
    }

    // destroy video
    //

    if( cr_video )
    {
        cairo_destroy( cr_video );
        cr_video = NULL;
    }

    if( cairo_surface_video != NULL)
    {
        cairo_surface_destroy( cairo_surface_video );
        cairo_surface_video = NULL;
    }
}

void video_decode_context::write_frame(AVFrame* frame_in, int start_at)
{
    AME_VIDEO_FRAME frame_out;

    int best_pts = av_frame_get_best_effort_timestamp(frame_in) * av_q2d(format_context_->streams[ stream_idx_ ]->time_base) * 1000;
    last_pts_ms_ = best_pts;
    //int pkt_pts = frame_in->pkt_dts * av_q2d(format_context_->streams[ stream_idx_ ]->time_base) * 1000;

    AVCodecContext* codec_context = get_codec_context();

    frame_out.width = buffer_->width_;
    frame_out.height = buffer_->height_;

    if(!sws_context_mix_ )
    {
        sws_context_mix_ = sws_getContext(frame_in->width, frame_in->height, (PixelFormat) frame_in->format, frame_out.width, frame_out.height, PIX_FMT_RGB32, SWS_BICUBIC, 0, 0, 0);
    }

    if( !sws_context_mix_ )
    {
        stringstream ss;
        ss << "failed to get scale context";
        throw app_fault( ss.str().c_str() );
    }

    //Scale the raw data/convert it to our video buffer...
    //
    frame_out.data[0] = frame_out.raw_data;
    frame_out.data[1] = NULL;
    frame_out.data[2] = NULL;
    frame_out.data[3] = NULL;

    frame_out.linesize[ 0 ] = 4 * frame_out.width;
    frame_out.linesize[ 1 ] = 0;
    frame_out.linesize[ 2 ] = 0;
    frame_out.linesize[ 3 ] = 0;

    sws_scale(sws_context_mix_, frame_in->data, frame_in->linesize, 0, frame_in->height, frame_out.data, frame_out.linesize);

    frame_out.pts_ms = best_pts;
    frame_out.start_at = start_at;

    // logger << "PTS:" << last_pts_ms_ << endl;

    frame_out.width = buffer_->width_;
    frame_out.height = buffer_->height_;
    frame_out.played_p = false;
    frame_out.skipped_p = false;

    with_cairo(&frame_out);

    int ret = vwriter<AME_VIDEO_FRAME>(buffer_, false)( &frame_out, 1);

    /*
    stringstream ss;
    ss << last_pts_ms_ / 1000 << " s";
    string time_str = ss.str();
    logger << "WRITE:" << last_pts_ms_ << " ms " << buffer_->get_available_samples()  << endl;
    */

    if( ret <= 0 ) throw decode_done_t();
}

void video_decode_context::test_assets()
{
   double alpha = 0.2;

   const char* markup = "<span font='New Century Schoolbook 48' foreground='#22ff22'> There are various combinations of <span foreground='#ffff00' style='italic'>fonts</span> and colors.  They need to be well layed out for an effective presentation.</span>";

   assets_.push_back( new text_asset_t(&scratch_pad_, markup, alpha, 0.2, 0.8, 0.4, 3000, -1, 100, 200, 250, 1200, -1) );
   assets_.push_back( new bitmap_asset_t(&scratch_pad_, "/mnt/MUSIC-THD/test-image-1.png", alpha, 0.2, 0.8, 0.4, 3000, 30000, 100, 400, 260, -1, -1) );
   markup = "<span font='Helvetica Bold 24' foreground='#dddd88'>  Explore the menus and experiment to see what works best.</span>";
   alpha = 1.0;
   assets_.push_back( new text_asset_t(&scratch_pad_, markup, alpha, 0.8, 0.2, 0.2, -1, 6000, 100, 250, 250, 800, -1) );
}

video_decode_context::video_decode_context(const char* mp4_file_path, ring_buffer_video_t* ring_buffer):decode_context(mp4_file_path, AVMEDIA_TYPE_VIDEO, &avcodec_decode_video2, ring_buffer->get_frames_per_period() * (ring_buffer->get_periods() - 1) - ring_buffer->get_available_samples()),buffer_( ring_buffer ), last_pts_ms_(0),sws_context_mix_(NULL), sws_context_out_(NULL)
{
    scratch_pad_.width = buffer_->width_;
    scratch_pad_.height = buffer_->height_;
    scratch_pad_.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, buffer_->width_, buffer_->height_);
    scratch_pad_.cr = cairo_create(scratch_pad_.surface);

    test_assets();
}

video_decode_context::~video_decode_context()
{
    if( sws_context_mix_ ) av_free( sws_context_mix_ );
    if( sws_context_out_ ) av_free( sws_context_out_ );

    for( list<asset_t*>::iterator it = assets_.begin(); it != assets_.end(); ++it )
    {
        delete (*it);
    }

    cairo_surface_destroy(scratch_pad_.surface);
    cairo_destroy(scratch_pad_.cr);
}

void video_decode_context::operator()(int start_at)
{
    try
    {
        iter_frames(start_at);
    }
    catch(decode_done_t& e)
    {
        vwriter<AME_VIDEO_FRAME>(buffer_, false).close();
    }
    catch(app_fault& e)
    {
        logger << e;
        vwriter<AME_VIDEO_FRAME>(buffer_, false).error();
    }
}



