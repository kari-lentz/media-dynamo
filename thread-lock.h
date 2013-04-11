template <typename T> class lock_t
{
private:
    T v_;
    pthread_mutex_t mutex_;

public:

lock_t():v_(0)
    {
        pthread_mutex_init(&mutex_, NULL);
    }

    lock_t(lock_t& lock)
    {
        pthread_mutex_init(&mutex_, NULL);

        pthread_mutex_lock(&mutex_);
        lock.v_ = lock.v;
        pthread_mutex_unlock(&mutex_);
    }

lock_t(T v):v_(v)
    {
        pthread_mutex_init(&mutex_, NULL);

        pthread_mutex_lock(&mutex_);
        v_ = v;
        pthread_mutex_unlock(&mutex_);
    }

    ~lock_t()
    {
        pthread_mutex_destroy(&mutex_);
    }

    operator const T()
    {
        T ret;

        pthread_mutex_lock(&mutex_);
        ret = v_;
        pthread_mutex_unlock(&mutex_);
        return ret;
    }

    void operator ()(T v)
    {
        pthread_mutex_lock(&mutex_);
        v_ = v;
        pthread_mutex_unlock(&mutex_);
    }

    lock_t& operator=(T v)
    {
        pthread_mutex_lock(&mutex_);
        v_ = v;
        pthread_mutex_unlock(&mutex_);

        return *this;
    }

    lock_t& operator=(lock_t& lock)
    {
        pthread_mutex_lock(&mutex_);
        v_ = lock.v_;
        pthread_mutex_unlock(&mutex_);

        return *this;
    }

};
