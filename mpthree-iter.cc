#include <fstream>
#include <iostream>
#include "mpthree-iter.h"

using namespace std;

unsigned long mp3_iter::extract_frame_header()
{
    unsigned long ulret = 0x00;
    int ndx;
    int nshift;

    for( ndx = 0; ndx < 4; ndx++ )
    {
        nshift = (3 - ndx) * 8;
        ulret |= ( 0x000000ff << nshift )  & ( ( (unsigned long) pcur_[ndx] ) << nshift );
    }

    return ulret;
}

unsigned long mp3_iter::extract_freq( unsigned long ulframe_header )
{
    const unsigned long kulmask_sampling = 0x00000C00; //the mask forthe bits that contain the sampling rate
    unsigned long ultemp; //temporary variable
    unsigned long ulfreq; //the pcm sampling frequecy

    ultemp = ( ulframe_header & kulmask_sampling ) >> 10;

    switch( ultemp )
    {
    case 0x00:
    {
	ulfreq = 44100;
	break;
    }
    case 0x01:
    {
	ulfreq = 48000;
	break;
    }
    case 0x02:
    {
	ulfreq = 32000;
	break;
    }
    default:
    {
	ulfreq = 0;
	break;
    }
    }

    return ulfreq;
}

unsigned long mp3_iter::extract_bit_rate( unsigned long ulframe_header )
{
    unsigned long ulbit_rate; // array of bit rates
    unsigned long ultemp; //temporary variable
    unsigned long ulbit_rates [] = { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128,160, 192, 244, 256, 320, 0 };
    const unsigned long ulmask_bit_rate = 0x0000F000; //the mask for the bits contain the bitrate

    ultemp = (ulframe_header & ulmask_bit_rate ) >> 12;

    if( ultemp >= 0 && ultemp < sizeof(ulbit_rates) )
    {
        ulbit_rate = ulbit_rates[ ultemp ] * 1000;
    }
    else
    {
        ulbit_rate = 0;
    }

    return ulbit_rate;
}

unsigned long mp3_iter::extract_frame_size( unsigned long ulframe_header )
{
    unsigned long ulbit_rate; // the bit rate in the frame
    unsigned long ulfreq; //the finally frequency track is played at
    unsigned long ulframe_size; //the size of the mp3
    unsigned long kulpadding = 0x00000200;

    ulbit_rate = extract_bit_rate( ulframe_header );

    if( ulbit_rate == 0 )
    {
        ulframe_size = 0;
    }

    ulfreq = extract_freq( ulframe_header );

    if( ulfreq > 0 )
    {
        ulframe_size = 144 * ulbit_rate / ulfreq + ( ulframe_header & kulpadding ? 1 : 0 );
    }
    else
    {
        ulframe_size = 0;
    }

    return ulframe_size;
}

bool mp3_iter::verify_frame_header( unsigned long ulframe_header )
{
    return (ulframe_header & kulframe_header_) == kulframe_header_;
}

mp3_iter::mp3_iter( const unsigned char* pbegin, size_t len )
{
    pbegin_ = pbegin;
    pcur_ = pbegin_;
    len_ = len;
}

mp3_iter& mp3_iter::operator++()
{
    unsigned long ulheader =  extract_frame_header();
    unsigned long ulsize;

    ulheader =  extract_frame_header();
    if( verify_frame_header( ulheader ) )
    {
        ulsize = extract_frame_size( ulheader );
        if( ulsize > 0 )
	{
            pcur_ += ulsize;

            if( pcur_ - pbegin_ >= len_ )
	    {
                pcur_ = NULL;
	    }
	}
        else
	{
            pcur_ = NULL;
	}
    }
    else
    {
        pcur_ = NULL;
    }

    return *this;
}

ostream& operator<<(ostream& os, mp3_iter& it)
{
    unsigned long ulheader = it.extract_frame_header();
    if( it.verify_frame_header( ulheader ) )
    {
        os << "freq:" << it.extract_freq( ulheader ) << endl;
        os << "bit rate:" << it.extract_bit_rate( ulheader ) << endl;
        os << "frame_size:" << it.extract_frame_size( ulheader ) << endl;
    }
    else
    {
        os << "not on valid frame header" << endl;
    }
}

unsigned long mp3_iter::get_full_frame_bytes()
{
    unsigned long ultotal = 0;
    unsigned long ulframe_size = 0;
    const unsigned char* pcur = pcur_;

    begin();

    while( (*this) != NULL )
    {
        ultotal += ulframe_size;
        ulframe_size = get_frame_size();
        ++(*this);
    }

    pcur = pcur_;

    return ultotal;
}

