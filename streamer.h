#ifndef STREAMER_H
#define STREAMER_H

template <typename T> class streamer
{
 public:
  virtual int operator()(T* pbuffer, int nsize) = 0;
};

template <typename OBJ, typename T> class specific_streamer:public streamer<T>
{
 private:
  OBJ* pobj_;
  int (OBJ::*pfn_)(T* pbuffer, int nsize);

 public:

  specific_streamer()
    {
      pobj_ = NULL;
      pfn_ = NULL;
    }

  specific_streamer( OBJ* pobj, int (OBJ::*pfn)(T* pbuffer, int nsize) )
  {
    pobj_ = pobj;
    pfn_ = pfn;
  }

  int operator()(T* pbuffer, int nsize)
  {
    return (*pobj_.*pfn_) (pbuffer, nsize );
  }
};

#endif
