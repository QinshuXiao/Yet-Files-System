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
    //VERIFY(pthread_cond_init(&_wc, 0) == 0);
}

lock_server::~lock_server()
{
    VERIFY(pthread_mutex_destroy(&_ol) == 0);
    //VERIFY(pthread_cond_destroy(&_wc) == 0);
    for(auto it = l_t.begin(); it != l_t.end(); ++it){
        delete it->second;
    }
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
    unordered_map<lock_protocol::lockid_t, lock*>::iterator index = l_t.find(lid);
    if(index == l_t.end()){
        lock* nl = new lock(lid);
        nl->l_status = lock::LOCKED;
        l_t[lid] = nl;
        r = ++nacquire;
        pthread_mutex_unlock(&_ol);
        lock_protocol::status ret = lock_protocol::OK;
        printf("ACQUIRE DONE: successfully acquiring the lock (lock_id:%016llx)!\n", lid);
        return ret;
    }
    else{
        lock* lt = index->second;
        while(lt->l_status == lock::LOCKED){
            pthread_cond_wait(&lt->_wc, &_ol);
        }
        lt->l_status = lock::LOCKED;
        r = ++nacquire;
        pthread_mutex_unlock(&_ol);
        lock_protocol::status ret = lock_protocol::OK;
        printf("ACQUIRE DONE: successfully acquiring the lock (lock_id: %016llx)!\n", lid);
        return ret;
    }
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
    pthread_mutex_lock(&_ol);
    unordered_map<lock_protocol::lockid_t, lock*>::iterator index = l_t.find(lid);
    if(index == l_t.end()){
        r = nacquire;
        pthread_mutex_unlock(&_ol);
        lock_protocol::status ret = lock_protocol::NOLOCK;
        printf("ERROR: No such lock (lock_id:%016llx) exists in the serve!\n", lid);
        return ret;
    }
    
    lock* lt = index->second;
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
    lock_protocol::status ret = lock_protocol::OK;

    return ret;
}
