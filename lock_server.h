// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include <unordered_map>
#include <memory>
#include <utility>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"
#include <pthread.h>
using namespace std;

class lock{
    public:
        enum lock_status {FREE, LOCKED};
        int l_status;
        pthread_cond_t _wc;
        lock_protocol::lockid_t lid;
        lock(lock_protocol::lockid_t _lid) : lid(_lid){
            VERIFY(pthread_cond_init(&_wc, 0) == 0);
        }
        ~lock(){
            VERIFY(pthread_cond_destroy(&_wc) == 0);
        }
};

class lock_server {

 protected:
  int nacquire;
  
  unordered_map<lock_protocol::lockid_t, std::shared_ptr<lock> > l_t;
  pthread_mutex_t _ol;

 public:
  lock_server();
  ~lock_server();
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status remove(int clt, lock_protocol::lockid_t lid, int &);
};

#endif 







