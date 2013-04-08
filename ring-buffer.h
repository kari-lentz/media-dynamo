#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <pthread.h>

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

template <typename T> class ring_buffer
{
 private:

  int frames_per_period_;
  int buffer_size_;

  T* pv_;
  T* ptemp_;
  int read_ptr_;
  int write_ptr_;
  bool full_p_;
  bool eof_p_;
  bool error_p_;

  pthread_mutex_t mutex_;
  pthread_cond_t cond_empty_;
  pthread_cond_t cond_full_;

  int get_samples_avail()
  {
    return full_p_ ? buffer_size_ : ( write_ptr_ >= read_ptr_ ? write_ptr_ - read_ptr_ : buffer_size_ - read_ptr_ + write_ptr_ );
  }

 public:

ring_buffer()
{
    frames_per_period_ = 0;
    buffer_size_ = 0;

    eof_p_ = false;
    error_p_ = false;
    read_ptr_ = 0;
    write_ptr_ = 0;
    full_p_ = false;
    pv_ = NULL;
    ptemp_ = NULL;

    pthread_mutex_init(&mutex_, NULL);

    pthread_cond_init(&cond_empty_, NULL);
    pthread_cond_init(&cond_full_, NULL );
}

ring_buffer( size_t frames_per_period, size_t periods):frames_per_period_(frames_per_period), buffer_size_( frames_per_period * periods ), eof_p_( false ),  error_p_( false )
  {
    pv_ = new T[ buffer_size_  ];
    ptemp_ = new T[ frames_per_period_ ];

    read_ptr_ = 0;
    write_ptr_ = 0;
    full_p_ = false;

    pthread_mutex_init(&mutex_, NULL);

    pthread_cond_init(&cond_empty_, NULL);
    pthread_cond_init(&cond_full_, NULL );
  }

 ~ring_buffer()
   {
     pthread_mutex_destroy(&mutex_);

     pthread_cond_destroy(&cond_empty_);
     pthread_cond_destroy(&cond_full_ );

     if( pv_ )
       {
	 delete [] pv_;
       }

     if( ptemp_ )
       {
	 delete [] ptemp_;
       }
   }

 int get_frames_per_period()
 {
   return frames_per_period_;
 }

 int read_avail( streamer<T>* pfnreader )
 {
   pthread_mutex_lock( &mutex_ );

   if( !error_p_ && !eof_p_ && get_samples_avail() == 0 )
     {
       pthread_cond_wait( &cond_empty_, &mutex_ );
     }

   int write_ptr = write_ptr_;
   bool error_p = error_p_;
   bool empty_p = get_samples_avail() == 0;
   bool eof_p = eof_p_;

   pthread_mutex_unlock( &mutex_ );

   int samples = 0;

   if( error_p )
     {
       return -1;
     }
   else if( !empty_p )
     {
       if( write_ptr > read_ptr_ )
	 {
	   samples = (*pfnreader )( pv_ + read_ptr_, write_ptr - read_ptr_ );
	 }
       else
	 {
	   samples = (*pfnreader )( pv_ + read_ptr_, buffer_size_ - read_ptr_ );

	   if(samples > 0 && write_ptr > 0)
	     {
	       samples += (*pfnreader )( pv_, write_ptr );
	     }
	 }
     }
   else
     {
       if( eof_p )
	 {
	   return 0;
	 }
     }

   pthread_mutex_lock( &mutex_ );

   if( samples < 0 )
     {
       error_p_ = true;
     }
   else if( samples > 0 )
     {
       full_p_ = false;

       read_ptr_ = (read_ptr_ + samples) % buffer_size_;
     }

   // buffer still considered full if write pointer was not far enough ahead
   //
   if( error_p_ || buffer_size_ - get_samples_avail() >=  frames_per_period_ )
     {
       pthread_cond_signal( &cond_full_ );
     }

   pthread_mutex_unlock( &mutex_ );

   return samples;
 }

int write_period( streamer<T>* pfnwriter )
 {
     pthread_mutex_lock( &mutex_ );

     // buffer "full"
     //
     if( !error_p_ && ( buffer_size_ - get_samples_avail() ) < frames_per_period_ )
     {
         pthread_cond_wait( &cond_full_, &mutex_ );
     }

     bool error_p = error_p_;

     pthread_mutex_unlock( &mutex_ );

     if( error_p )
     {
         return -1;
     }

     int samples = (*pfnwriter)( ptemp_, frames_per_period_  );

     if( samples > 0 )
     {
         if( write_ptr_ + samples < buffer_size_ )
         {
             memcpy(pv_ + write_ptr_, ptemp_, samples * sizeof(T) );
         }
         else
         {
             memcpy(pv_ + write_ptr_, ptemp_, (buffer_size_ - write_ptr_) * sizeof(T) );
             memcpy(pv_, ptemp_ + (buffer_size_ - write_ptr_) * sizeof(T), (write_ptr_ + samples - buffer_size_) * sizeof(T) );
         }
     }

     pthread_mutex_lock( &mutex_ );

     if( samples > 0 )
     {
         write_ptr_ = (write_ptr_ + samples) % buffer_size_;

         if(write_ptr_ == read_ptr_)
         {
             full_p_ = true;
         }
     }
     else if( samples == 0 )
     {
         eof_p_ = true;
     }
     else
     {
         error_p_ = true;
     }

     pthread_cond_signal( &cond_empty_  );

     pthread_mutex_unlock( &mutex_ );

     return samples;
 }

};

#endif
