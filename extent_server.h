// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include <unordered_map>
#include <utility>
#include <random>
#include <ctime>
#include <memory>
#include <unistd.h>
#include "extent_protocol.h"
#include "lang/verify.h"
#include "lang/algorithm.h"


struct extent_t{
    extent_protocol::extentid_t eid;
    extent_protocol::extentid_t parent_eid;

    std::string name;
    
    std::map<std::string, extent_protocol::extentid_t> file_entry;

    std::string buf;
    extent_protocol::attr attr;
    //pthread_mutex_t eo;
    //pthread_cond_t ec;
    
    extent_t() = default;

    extent_t(extent_protocol::extentid_t _eid, extent_protocol::extentid_t _peid, std::string _name, int sz = 0):eid(_eid), parent_eid(_peid), name(_name){
        attr.atime = attr.ctime = attr.mtime = std::time(nullptr);
        attr.size = sz;
        //VERIFY(pthread_mutex_init(&eo, 0) == 0);
        //VERIFY(pthread_cond_init(&ec, 0) == 0);
    }

    extent_t(const extent_t &_extent):eid(_extent.eid), parent_eid(_extent.parent_eid), name(_extent.name), buf(_extent.buf) {}

    extent_t(extent_t &&rhs):eid(rhs.eid), parent_eid(rhs.parent_eid), name(std::move(rhs.name)), file_entry(rhs.file_entry), attr(rhs.attr) {}

    extent_t& operator=(const extent_t &lhs){
        if(&lhs != this){
            eid = lhs.eid;
            parent_eid = lhs.parent_eid;
            name = lhs.name;
            file_entry = lhs.file_entry;
            attr = lhs.attr;
        }
        return *this;
    }
};

class extent_server {

  public:
    extent_server();
    ~extent_server();
    int put(extent_protocol::extentid_t id, std::string, int &);
    int get(extent_protocol::extentid_t id, std::string &);
    int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
    int remove(extent_protocol::extentid_t id, int &);
    int lookup(extent_protocol::extentid_t parent_eid, std::string name, extent_protocol::extentid_t &ret);
    int create(extent_protocol::extentid_t parent_eid, std::string name, extent_protocol::extentid_t &ret);
    int readdir(extent_protocol::extentid_t id, std::map<std::string, extent_protocol::extentid_t> &ret);

  private:
    std::unordered_map<extent_protocol::extentid_t , std::shared_ptr<extent_t> > ext_map;
    pthread_mutex_t _m;//protect the map operation
    bool isfile(extent_protocol::extentid_t id){
        return (id & 0x80000000) != 0; 
    }

    std::default_random_engine rd_gen;
};

#endif 

