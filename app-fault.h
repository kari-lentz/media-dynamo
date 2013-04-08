#ifndef APP_FAULT_H
#define APP_FAULT_H

#include <string>
#include <fstream>
#include <iostream>
#include "null-stream.h"

using namespace std;

//#define _THD_DEBUG

#ifndef _THD_DEBUG
#define caux my_null_stream
//#define caux cerr
#else
#define caux log_error
#endif

class app_fault
{
private:

    string message_;

public:

    app_fault(const char* message):message_(message)
    {
    }

    friend ostream& operator << (ostream& os, app_fault& my_app_fault)
    {
        os << my_app_fault.message_ << endl;
        return os;
    }
};

#endif
