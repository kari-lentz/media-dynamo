
#ifndef RESULT_H
#define RESULT_H

#include "logger.h"

using namespace std;

class result
{
protected:
    string sop_;
    int iresult_;

public:

result():iresult_(0){}

result( const char* szop, int iresult):sop_( szop ), iresult_(iresult){}

    operator bool ()
    {
        return (iresult_ >= 0) ? true : false;
    }

    operator int ()
    {
        return iresult_;
    }

    virtual operator string ()
    {
        return string( iresult_ >= 0  ? "Ok": "general error" );
    }

    friend stringstream& operator <<(stringstream& ss, result& r)
    {
        ss << r.sop_ << ": " << ( (string) r );
        return ss;
    }

    /*
      friend ostream& operator <<(ostream& os, result& r)
      {
      os << r.sop_ << ": " << ( (string) r );
      return os;
      } */


    bool operator()( const char* szop, int iresult, const char* szmisc = NULL)
    {
        sop_ = szop;
        iresult_ = iresult;

        return (iresult_ >= 0) ? true : false;
    }
};

#endif
