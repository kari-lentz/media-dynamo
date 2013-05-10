#ifndef SYNCH_H
#define SYNCH_H

template <typename T> class synch_t
{
private:
    T v_;
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;

public:

synch_t():v_(0)
    {
        pthread_mutex_init(&mutex_, NULL);
        pthread_cond_init(&cond_, NULL);
    }

synch_t(T v):v_(v)
    {
        pthread_mutex_init(&mutex_, NULL);
        pthread_cond_init(&cond_, NULL);
    }

    ~synch_t()
    {
        pthread_cond_destroy(&cond_);
        pthread_mutex_destroy(&mutex_);
    }

    T wait()
    {
        T ret;

        pthread_mutex_lock(&mutex_);
        ret = v_;
        if(!ret)
        {
            pthread_cond_wait(&cond_, &mutex_);
            ret = v_;
        }

        pthread_mutex_unlock(&mutex_);

        return ret;
    }

    void signal(T v)
    {
        pthread_mutex_lock(&mutex_);
        v_ = v;
        pthread_cond_signal(&cond_);
        pthread_mutex_unlock(&mutex_);
    }

    void broadcast(T v)
    {
        pthread_mutex_lock(&mutex_);
        v_ = v;
        pthread_cond_broadcast(&cond_);
        pthread_mutex_unlock(&mutex_);
    }

};

typedef synch_t<bool> ready_synch_t;

#endif
