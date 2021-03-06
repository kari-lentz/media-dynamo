#ifndef ENV_WRITER_H
#define ENV_WRITER_H

#include "ring-buffer-video.h"

typedef struct
{
  const char* pf_;
  ring_buffer_video_t* pbuffer_;
  int start_at_;
} env_writer_t;

#endif
