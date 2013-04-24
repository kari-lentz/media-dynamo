
#ifndef ALSA_RESULT_H
#define ALSA_RESULT_H

#include "result.h"
#include <alsa/asoundlib.h>

using namespace std;

class alsa_result : public result
{
public:

    const static int kmisc_ = -32768;
    string smisc_;

alsa_result():result(){}
alsa_result( const char* szdoc, int ires ): result( szdoc, ires ){};

    operator string ()
    {
        stringstream sstr;
        sstr << "ALSA:" << (smisc_.empty() ?  ( iresult_ >= 0 ? " Ok" : snd_strerror( iresult_ ) ) : smisc_);
        return sstr.str();
    }

    bool operator()( const char* szop, int iresult, const char* szmisc = NULL)
    {
        sop_ = szop;
        iresult_ = iresult;
        smisc_ = szmisc ? szmisc : "";

        return (iresult_ >= 0) ? true : false;
    }

    operator bool ()
    {
        return (iresult_ >= 0) ? true : false;
    }

    operator int ()
    {
        return iresult_;
    }

};

#endif
