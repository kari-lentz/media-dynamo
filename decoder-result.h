#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <string>
#include <mpg123.h>
#include "smart-ptr.h"
#include "result.h"

#ifndef DECODE_RESULT_H
#define DECODE_RESULT_H

class decoder_result:public result
{
    mpg123_handle* mh_;
    int nchannel_;

public:

    static const int ALL_CHANNEL = -1;

decoder_result():result(), mh_(NULL), nchannel_(-1){}

decoder_result(int nchannel):result(), mh_(NULL), nchannel_(nchannel){}

decoder_result( int nchannel, const char* szdoc, int iresult, mpg123_handle* mh = NULL ):result(szdoc, iresult), nchannel_(nchannel), mh_(mh){}

    operator string ()
    {
        stringstream sstr;
        if( nchannel_ >= 0 ) sstr << "channel: " << nchannel_ + 1 << " ";
        sstr << (iresult_ == MPG123_OK ? "Ok" : ( iresult_ == MPG123_NEED_MORE ? "Need More" : ( !mh_ ? mpg123_plain_strerror(iresult_) : mpg123_strerror(mh_) ) ) );

        return sstr.str();
    }

    decoder_result& operator = (mpg123_handle* mh)
                               {
                                   mh_ = mh;
                                   return *this;
                               }

    operator mpg123_handle* ()
    {
        return mh_;
    }

    operator bool()
    {
        return ( iresult_ == MPG123_OK ) || (iresult_ == MPG123_NEED_MORE);
    }

    bool operator()( const char* szdoc, int iresult )
    {
        sop_ =  szdoc;
        iresult_ = iresult;
        return iresult == MPG123_OK ? true : false;
    }

    bool operator()( const char* szdoc, mpg123_handle* mh, int* piresult  )
    {
        sop_ = szdoc;
        if( mh ) mh_ = mh;
        int iresult = piresult ? *piresult : MPG123_BAD_VALUE;
        return iresult == MPG123_OK ? true : false;
    }
};

#endif
