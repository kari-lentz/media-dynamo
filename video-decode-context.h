#ifndef VIDEO_DECODE_CONTEXT_H
#define VIDEO_DECODE_CONTEXT_H

#include <list>
#include <SDL/SDL.h>
#include "decode-context.h"
#include "ring-buffer-video.h"
#include "vwriter.h"
#include "synch.h"
#include "cairo-f.h"
#include "asset.h"

using namespace std;

typedef struct
{
    const char* mp4_file_path;
    int start_at;
    ring_buffer_video_t* ring_buffer;
    bool debug_p;
    bool run_p;
    int ret;
} env_video_decode_context;

class video_decode_context:public decode_context<AME_VIDEO_FRAME>
{
private:
    ring_buffer_video_t* buffer_;

    AVFormatContext* oc_;
    FILE* outfile_;
    list<asset_t*> assets_;
    int last_pts_ms_;
    scratch_pad_t scratch_pad_;

    SwsContext*  sws_context_mix_;
    SwsContext*  sws_context_out_;

    void load_cairo_commands();
    void unload_cairo_commands();

    void run_commands(cairo_t* cr, int media_ms);

    void using_cairo(AME_VIDEO_FRAME* frame, cairo_t* cr);
    void with_cairo(AME_VIDEO_FRAME* frame);

    void buffer_primed();
    void write_frame(AVFrame* frame_in);

    void test_assets();

public:

    video_decode_context(const char* mp4_file_path, ring_buffer_video_t* ring_buffer);
    ~video_decode_context();

    void operator()(int start_at = 0);
};

#endif
