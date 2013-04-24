#ifndef SMART_PTR_H
#define SMART_PTR_H

#include <stdio.h>
#include <stdarg.h>
#include <ostream>
#include "streamer.h"
#include "stream-max-writer.h"

using namespace std;

class ref_ctr
{
private:
    int m_nref;

public:
    ref_ctr()
    {
        m_nref = 0;
    }

    void inc()
    {
        m_nref++;
    }

    void dec()
    {
        m_nref--;

        if( m_nref == 0 )
        {
            delete this;
        }
    }

    virtual ~ref_ctr()
    {
    }
};

template <typename T> class ptr
{
private:
    T* m_pr;

public:

    ptr()
    {
        m_pr = NULL;
    }

    ptr( T* pr )
    {
        m_pr = pr;
        if( m_pr ) m_pr->inc();
    }

    ptr( const ptr& another )
    {
        m_pr = another.m_pr;
        if( m_pr ) m_pr->inc();
    }

    ~ptr()
    {
        if( m_pr ) m_pr->dec();
    }

    ptr& operator=( const ptr& another)
                  {
                      if( this == &another ) return *this;

                      if( m_pr )  m_pr->dec();

                      m_pr = another.m_pr;

                      if( m_pr ) m_pr->inc();
                      return *this;
                  }

    void operator=( T* p )
                  {
                      if( m_pr ) m_pr->dec();
                      m_pr = p;
                      if( m_pr ) m_pr->inc();
                  }

    operator bool ()
    {
        return m_pr != NULL ? true : false;
    }

    operator T* ()
    {
        return m_pr;
    }

    T *operator->() const
    {
        return m_pr;
    }
};

template <typename T> class stream_printer
{
    static ostream& call( ostream& os, T* pv, int len)
    {
        for( int nidx = 0; nidx < len; nidx++ )
        {
            os << *pv[ nidx ];
        }

        return os;
    }
};

template <> class stream_printer<char>
{
public:
    static ostream& call( ostream& os, char* pv, int len)
    {
        os.write( pv, len );
        return os;
    }
};

template <> class stream_printer<unsigned char>
{
public:
    static ostream& call( ostream& os, unsigned char* pv, int len)
    {
        os.write( (char*) pv, len );
        return os;
    }
};


template <typename T> class packed_array:public ref_ctr
{
    T* pv_;
    size_t len_;
    size_t cap_;
    T* ptemp_;

    size_t calc_len( const T* pv )
    {
        size_t len = 0;

        if( pv )
	{
            for( const T* p = pv; *p != T(0); p++ )
	    {
                len++;
	    }
	}

        return len;
    }

    size_t idx_begin( int n )
    {
        return ( n >= 0 ) ? n : ( len_ + n );
    }

    size_t idx_end( int n )
    {
        return ( n > 0 ) ? n : ( len_ + n);
    }

    const T* element_begin( int n )
    {
        return &pv_[ idx_begin( n ) ];
    }

    const T* element_end( int n )
    {
        return &pv_[ idx_end( n ) ];
    }

    packed_array& concat(const T* pv_begin, const T* pv_end )
    {
        size_t len_param = pv_end - pv_begin;
        size_t len_new = len_ + len_param;

        if( len_new > cap_  )
	{
            cap_ = len_new;

            T* pv_old = pv_;
            pv_ = new T[ cap_ ];

            if( len_ > 0  )
	    {
                memcpy( pv_, pv_old, len_ * sizeof( T ) );
	    }

            if( pv_old )
	    {
                delete [] pv_old;
	    }
	}

        if( len_param > 0 )
	{
            memcpy( pv_ + len_, pv_begin, len_param * sizeof(T) );
	}

        len_ = len_new;

        return *this;
    }

    packed_array& copyfrom( const T* pv_begin, const T* pv_end )
    {
        len_ = 0;
        return concat( pv_begin, pv_end );
    }

public:

    packed_array()
    {
        pv_ = NULL;
        len_ = 0;
        cap_ = 0;
        ptemp_ = NULL;
    }

    ~packed_array()
    {
        if( pv_ )
	{
            delete [] pv_;
            pv_ = NULL;
	}

        if( ptemp_ )
	{
            delete [] ptemp_;
            ptemp_ = NULL;
	}

        len_ = 0;
        cap_ = 0;
    }

    packed_array( const packed_array& pa )
    {
        len_ = pa.len_;
        cap_ = pa.cap_;

        if( cap_ > 0 )
	{
            pv_ = new T[ cap_  ];
	}

        if( len_ > 0 )
	{
            memcpy( pv_, pa.pv_, len_ * sizeof(T) );
	}

        ptemp_ = NULL;
    }

    packed_array(const T* pv)
    {
        const T* p;
        size_t len = calc_len( pv );

        len_ = len;
        cap_ = len_;

        if( cap_ > 0 )
	{
            pv_ = new T[ cap_ ];
            memcpy( pv_, pv, len_ * sizeof(T) );
	}
        else
	{
            pv_ = NULL;
	}

        ptemp_ = NULL;
    }

    packed_array( int cap )
    {
        len_ = 0;
        cap_ = cap;

        pv_ = new T[ cap_ ];

        ptemp_ = NULL;
    }

    size_t len()
    {
        return len_;
    }

    size_t cap()
    {
        return cap_;
    }

    int resize( size_t len )
    {
        if( len > cap_ )
        {
            cap_ = len;
            T* pv = new T[ cap_ ];
            if( pv_ )
            {
                memcpy( pv, pv_, len_ );
                delete [] pv_;
            }
            pv_ = pv;
        }

        len_ = len;
    }

    operator const T* ()
    {
        if( ptemp_ )
        {
            delete [] ptemp_;
        }

        ptemp_ = new T [len_ + 1];
        if( len_ > 0 ) memcpy( ptemp_, pv_, len_ );
        ptemp_[ len_ ] = T(0);

        return ptemp_;
    }

    T& operator[]( const int n )
    {
        return pv_[ idx_begin( n ) ];
    }

    packed_array& concat( packed_array& pa, int begin = 0, int end = 0)
    {
        return concat( pa.element_begin( begin ), pa.element_end( end ) );
    }

    packed_array& copyfrom( packed_array& pa, int begin = 0, int end = 0 )
    {
        return copyfrom( pa.element_begin( begin ), pa.element_end( end ) );
    }

    void copyto( T* pbuffer, int begin = 0, int end = 0 )
    {
        memcpy( pbuffer,  pv_ + idx_begin( begin ), idx_end( end ) - idx_begin( begin ) );
    }

    packed_array& operator = ( packed_array& pa )
                             {
                                 return copyfrom( pa );
                             }

    packed_array& operator = ( const T* pv )
                             {
                                 size_t len = calc_len( pv );
                                 return copyfrom( pv, pv + len );
                             }

    packed_array& fill_to_cap( streamer<T>* pfnwriter )
    {
        while( len_ < cap_ )
	{
            size_t temp = (*pfnwriter)( pv_ + len_, cap_ - len_ );
            if( temp <= 0 ) break;
            len_ += temp;
	}

        return *this;
    }

    int copy_max( stream_max_writer* pfnwriter )
    {
        return (*pfnwriter)( pv_, cap_, &len_ );
    }

    packed_array& operator << (const T* pv)
    {
        size_t len = packed_array::calc_len( pv );
        concat( pv, pv + len );
        return *this;
    }

};

template <typename T> class cons:public ref_ctr
{
protected:
    T car_;
    cons* cdr_;

public:

    cons( cons* pnext )
    {
        cdr_ = pnext;
    }

    cons( const T& v, cons* pnext )
    {
        car_ = v;
        cdr_ = pnext;
    }

    T& car()
    {
        return car_;
    }

    cons* cdr()
    {
        return cdr_;
    }

    void destroy()
    {
        if( cdr_ )
        {
            cdr_->destroy();
        }

        delete this;
    }

    int len()
    {
        if( cdr_ )
        {
            return cdr_->len() + 1;
        }
        else
        {
            return 1;
        }
    }

    cons* reverse()
    {
        if( cdr_ )
	{
            cons* pnew_front = cdr_->reverse();
            cdr_->cdr_ = this;
            cdr_ = NULL;
            return pnew_front;
	}
        else
	{
            return this;
	}
    }
};

class pool
{
protected:

    char* m_pbytes;

public:

    pool()
    {
    }

    pool( int num_bytes )
    {
        m_pbytes = new char[ num_bytes ];
    }

    virtual ~pool()
    {
        if( m_pbytes )
        {
            delete [] m_pbytes;
        }
    }

    virtual char* operator()(int num_bytes) = 0;
};

template <typename T> class arena:public pool
{
private:
    int m_nsize;
    int m_nused;

public:

    arena()
    {
        m_nsize = 0;
        m_nused = 0;
    }

arena( int num_units ):pool( sizeof(T) * num_units )
    {
        m_nsize = sizeof(T) * num_units;
        m_nused = 0;
    }

    T* operator()(int num_units)
    {
        T* pv;
        int nused  = m_nused + (num_units * sizeof(T));

        if( nused <= m_nsize )
        {
            pv = (T*) (m_pbytes + nused - 1);
            m_nused = nused;
        }
        else
        {
            pv = NULL;
        }

        return pv;
    }

    ~arena()
    {
        m_nused = 0;
        m_nsize = 0;
    }

};

template <typename T> class interior_list_node
{
private:
    T m_v;
    interior_list_node* m_pnext;

public:

    void *operator new( size_t num_bytes, pool& pool)
    {
        return (pool)( num_bytes );
    }
};

template <typename T> class interior_list:public ref_ctr
{
private:
    pool* m_ppool;
    interior_list_node<T*> m_pbegin;

public:

    static pool* make_arena( int nsize )
    {
        return new arena< interior_list_node<T> >( nsize );
    }

    interior_list()
    {
        m_pbegin = NULL;
        m_ppool = NULL;
    }

    interior_list(interior_list_node<T>* pbegin, pool* ppool = NULL)
    {
        m_pbegin = pbegin;
        m_ppool = ppool;
    }

    ~interior_list()
    {
        if( m_ppool )
        {
            delete m_ppool;
            m_ppool = NULL;
        }
    }

    void sort( interior_list_node<T>* pn_first, bool (*pfncompare)(T* pv0, T* pv1)  )
    {
        interior_list_node<T>* pn_prev = pn_first;
        interior_list_node<T>* pn = pn_first->m_pnext;
        interior_list_node<T> *pn0 = NULL, pn1 = NULL; // first element of 1st half of recursion

        while( pn )
        {
            if( !(*pfncompare)( pn_first->m_v, pn->m_v ) )
            {
                // found an element that is lower ... move to bottom of list
                //
                pn_prev->m_pnext = pn->m_pnext;
                pn->m_pnext = pn_first;
                pn0 = pn; // this might start next search
            }
            pn_prev = pn;
            pn = pn->m_pnext;
        }

        pn1 = pn_first->m_pnext;

        if( pn0 )
        {
            pn_first->m_pnext = NULL;
            sort( pn0, pfncompare );
        }

        if( pn1 )
        {
            pn_first->m_pnext = NULL;
            sort( pn1 );
        }
    }
};

#endif
