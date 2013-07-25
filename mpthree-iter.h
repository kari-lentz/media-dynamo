#ifndef MP3_ITER_H
#define MP3_ITER_H

#include <iostream>

using namespace std;

class mp3_iter
{
    static const unsigned long kulframe_header_ = 0xfffb0000;

    unsigned long extract_frame_header();
    unsigned long extract_freq( unsigned long ulframe_header );
    unsigned long extract_bit_rate( unsigned long ulframe_header );
    unsigned long extract_frame_size( unsigned long ulframe_header );
    bool verify_frame_header( unsigned long ulframe_header );

    const unsigned char* pbegin_;
    const unsigned char* pcur_;
    size_t len_;

public:

    mp3_iter( const unsigned char* pbegin, size_t len );

    const unsigned char* begin()
    {
        return pbegin_;
    }

    mp3_iter& operator++();

    mp3_iter operator++( int )
    {
        mp3_iter copy( *this );
        ++(*this);
        return copy;
    }

    operator const unsigned char*()
    {
        return pcur_;
    }

    unsigned long get_frame_size()
    {
        unsigned long ulheader = extract_frame_header();
        if( verify_frame_header( ulheader ) )
        {
            return extract_frame_size( ulheader );
        }
        else
        {
            return 0;
        }
    }

    unsigned long get_full_frame_bytes();

    friend ostream& operator<< (ostream& os, mp3_iter& it);
};

#endif
