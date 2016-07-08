// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server():
  nacquire (0)
{
    VERIFY(pthread_mutex_init(&_ol, 0) == 0);
}

lock_server::~lock_server()
{
    VERIFY(pthread_mutex_destroy(&_ol) == 0);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
    pthread_mutex_lock(&_ol);
    lock_protocol::status ret = lock_protocol::OK;
    auto index = l_t.find(lid);
    if(index == l_t.end()){
        l_t[lid] = std::make_shared<lock>(lid); 
        l_t[lid]->l_status = lock::LOCKED;
        r = ++nacquire;
        pthread_mutex_unlock(&_ol);
        ret = lock_protocol::OK;
        printf("ACQUIRE DONE: successfully acquiring the lock (lock_id:%016llx)!\n", lid);
        return ret;
    }

    auto lt = index->second;
    while(lt->l_status == lock::LOCKED){
        printf("I'm waiting for the lock: %llx\n",lid);
        pthread_cond_wait(&lt->_wc, &_ol);
    }
    lt->l_status = lock::LOCKED;
    r = ++nacquire;
    pthread_mutex_unlock(&_ol);
    printf("ACQUIRE DONE: successfully acquiring the lock (lock_id: %016llx)!\n", lid);
    return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
    pthread_mutex_lock(&_ol);
    lock_protocol::status ret = lock_protocol::OK;
    auto index = l_t.find(lid);
    if(index == l_t.end()){
        r = nacquire;
        pthread_mutex_unlock(&_ol);
        ret = lock_protocol::NOLOCK;
        printf("ERROR: No such lock (lock_id:%016llx) exists in the serve!\n", lid);
        return ret;
    }
    
    auto lt = index->second;
    if(lt->l_status == lock::LOCKED){
        lt->l_status = lock::FREE;
        pthread_cond_signal(&lt->_wc);
        r = --nacquire;
        printf("RELEASE DONE: successfully releasing lock(lock_id:%016llx)!\n", lid);
    }
    else{
        r = nacquire;
        printf("WARNING: RE-releasing problem for the lock (lock_id: %016llx)!\n", lid);
    }
    pthread_mutex_unlock(&_ol);

    return ret;
}

lock_protocol::status
lock_server::remove(int clt, lock_protocol::lockid_t lid, int &r)
{
    pthread_mutex_lock(&_ol);
    auto index = l_t.find(lid);
    lock_protocol::status ret = lock_protocol::OK;

    if(index == l_t.end()){
        r = nacquire;
        pthread_mutex_unlock(&_ol);
        ret = lock_protocol::NOLOCK;
        printf("ERROR: No such lock (lock_id:%llx) exists in the server\n", lid);
        return ret;
    }

    auto lt = index->second;
    while(lt->l_status == lock::LOCKED){
        printf("I'm waiting for the lock %llx in remove\n", lid);
        pthread_cond_wait(&lt->_wc, &_ol);
    }

    l_t.erase(index);
    pthread_mutex_unlock(&_ol);
    printf("REMOVE DONE: successfully removing the lock (lock_id: %016llx)!\n", lid);
    return ret;
}
