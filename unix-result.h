#ifndef UNIX_RESULT_H
#define UNIX_RESULT_H

#include "result.h"

using namespace std;

class unix_result:public result
{

public:

unix_result():result(){}

unix_result( const char* szop, int iresult):result( szop, iresult >= 0 ? iresult : errno ){}

    operator const char* ()
    {
        return iresult_ >= 0 ? "Ok" : strerror( iresult_ );
    }
};

#endif
