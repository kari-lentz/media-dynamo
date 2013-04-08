#include "ring-buffer.h"

template <typename T> class vwriter
{
private:
  ring_buffer<T>* pbuffer_;
  specific_streamer<vwriter, T> functor_;
  T* src_buffer_;
  int remaining_;
  bool eof_p_;
  bool error_p_;

  int call(T* buffer, int size)
  {
      int ret;

      if(error_p_)
      {
          ret = -1;
      }
      else if(eof_p_)
      {
          ret = 0;
      }
      else
      {
          int write_size = remaining_ > size ? size : remaining_;

          int write_bytes = sizeof(T) * write_size;
          memcpy(buffer, src_buffer_,  write_bytes);
          remaining_ -= write_size;

          ret = write_size;
      }

      return ret;
  }

public:

vwriter(ring_buffer<T>* pbuffer):pbuffer_(pbuffer), functor_(this, &vwriter<T>::call), src_buffer_(NULL), remaining_(0), eof_p_(false),error_p_(false)
  {
  }

  ~vwriter()
  {
      //communicate eof to ring buffer
      //
      if( !error_p_ )
      {
          eof_p_ = true;
          pbuffer_->write_period( &functor_ );
      }
  }

  void write_error()
  {
      error_p_ = true;
      pbuffer_->write_period( &functor_ );
  }

  int operator ()(T* buffer, int size)
  {
    remaining_ = size;
    src_buffer_ = buffer;

    while(remaining_ > 0)
    {
	int ret = pbuffer_->write_period( &functor_ );
        if(ret <= 0) return ret;
    }

    return size;
  }
};
