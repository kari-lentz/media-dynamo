#ifndef MY_AUDIO_FRAME_H
#define MY_AUDIO_FRAME_H

typedef short BASIC_AUDIO_SAMPLE;

const int AUDIO_PACKET_SIZE = 1024;
const int RATE = 44100;
const int BITS_PER_SAMPLE = 16;

template  <int NUM_CHANNELS, int SAMPLES_PER_FRAME> struct ame_audio_frame_t
{
    bool skipped_p;
    bool played_p;
    int pts_ms;
    int start_at;
    int samples;
    BASIC_AUDIO_SAMPLE* data[1];
    BASIC_AUDIO_SAMPLE raw_data[ SAMPLES_PER_FRAME ][ NUM_CHANNELS ];
};

typedef struct ame_audio_frame_t<2, AUDIO_PACKET_SIZE> STEREO_AUDIO_FRAME;
typedef struct ame_audio_frame_t<1, AUDIO_PACKET_SIZE> AME_AUDIO_FRAME;

#endif
