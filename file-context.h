#ifndef ENCODER_CONTEXT_H
#define ENCODER_CONTEXT_H

#include "decode-context.h"

using namespace std;

class file_context:public decode_context
{
private:
    AVFormatContext* oc_;

    FILE* outfile_;


public:

    file_context(const char* mp4_file_path, ring_buffer_t* ring_buffer);
    ~file_context();

    void operator()(int start_at = 0);
};

#endif
