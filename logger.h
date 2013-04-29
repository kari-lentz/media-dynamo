#include <syslog.h>
#include <string.h>
#include <string>
#include <sstream>
#include <iostream>

#ifndef LOGGER_H
#define LOGGER_H

using namespace std;

class logger_base_t
{
    string identity_;
    int priority_;
    int facility_;
    int thread_id_;

    stringstream ss_;

    int call( const char* szmsg )
    {
        ::openlog( identity_.c_str(), LOG_PERROR, facility_ );
        ::syslog( priority_, "%s" ,szmsg );
        ::closelog();

        return 0;
    }

public:
logger_base_t( const char* identity, int zone, int priority, int facility ):priority_(priority), facility_(facility)
    {
        stringstream ss;

        ss << identity;
        if( zone > 0 )
	{
            ss << zone;
	}

        identity_ = ss.str();
        thread_id_ = pthread_self();
    }

    template <typename T> friend logger_base_t& operator << (logger_base_t& logger, const T& t)
    {
        logger.ss_ << t;
        return logger;
    }

    template <typename T> friend logger_base_t& operator << (logger_base_t& logger, T& t)
    {
        logger.ss_ << t;
        return logger;
    }

    friend logger_base_t& operator << (logger_base_t& logger, std::ostream& (*fn)(std::ostream&) )
    {
        logger.call( logger.ss_.str().c_str() );
        logger.ss_.~stringstream();
        new ( &logger.ss_ ) stringstream;

        return logger;
    }
};

class logger_t:public logger_base_t
{
public:
logger_t( const char* identity, int zone = 0 ):logger_base_t( identity, zone, LOG_INFO, LOG_LOCAL0 ){}
};

class warning_logger_t:public logger_base_t
{
public:
warning_logger_t( const char* identity, int zone = 0 ):logger_base_t( identity, zone, LOG_WARNING, LOG_LOCAL0 ){}
};

class err_logger_t:public logger_base_t
{
public:
err_logger_t( const char* identity, int zone = 0 ):logger_base_t( identity, zone, LOG_ERR, LOG_LOCAL0 ){}
};

#endif
