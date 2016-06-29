#ifndef __REMOTE_SCOPED_LOCK__
#define __REMOTE_SCOPED_LOCK__

#include "lock_client.h"

class RemoteScopedLock{
    private:
        lock_client* lc;
        lock_protocol::lockid_t lid;
    public:
        RemoteScopedLock(lock_client* _lc, lock_protocol::lockid_t _lid):lc(_lc), lid(_lid) {
            lc->acquire(lid);
        }

        ~RemoteScopedLock(){
            lc->release(lid);
        }
};

#endif
