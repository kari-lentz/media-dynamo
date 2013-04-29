
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

    friend ostream& operator << (ostream& os, alsa_result& ar)
    {
        os << "ALSA:" << (ar.smisc_.empty() ?  ( ar.iresult_ >= 0 ? " Ok" : snd_strerror( ar.iresult_ ) ) : ar.smisc_);
        return os;
    }

    friend stringstream& operator << (stringstream& ss, alsa_result& ar)
    {
        ss << "ALSA:" << (ar.smisc_.empty() ?  ( ar.iresult_ >= 0 ? " Ok" : snd_strerror( ar.iresult_ ) ) : ar.smisc_);
        return ss;
    }
};

#endif
