#ifndef STREAM_MAX_WRITER_H
#define STREAM_MAX_WRITER_H

class stream_max_writer
{
public:
    virtual int operator()(void* pbuffer, size_t requested, size_t* pactual ) = 0;
};

template <typename T> class specific_stream_max_writer:public stream_max_writer
{
private:
    T* pobj_;
    int (T::*pfn_)(void* pbuffer, size_t requested, size_t* pactual );

public:
    specific_stream_max_writer( T* pobj, int (T::*pfn)(void* pbuffer, size_t requested, size_t* pactual) )
    {
        pobj_ = pobj;
        pfn_ = pfn;
    }

    int operator()(void* pbuffer, size_t requested, size_t* pactual)
    {
        return (*pobj_.*pfn_) (pbuffer, requested, pactual );
    }
};

#endif
