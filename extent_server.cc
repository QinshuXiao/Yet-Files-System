// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extent_server::extent_server():rd_gen(std::time(0)) {
    VERIFY(pthread_mutex_init(&_m, 0) == 0);
    ext_map[1] = std::make_shared<extent_t>(1,0,"",0);
}

extent_server::~extent_server() {
    VERIFY(pthread_mutex_destroy(&_m) == 0);
}


int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  // You fill this in for Lab 2.
    printf("===================PUT=========================================\n");
    if(!isfile(id)){
        printf("The entity with id:%lld is directory, not file!\n", id);
        return extent_protocol::NOENT;
    }

    ScopedLock pl(&_m);
    
    auto ext_it = ext_map.find(id);
    if(ext_it == ext_map.end()){
        printf("No file exists with id:%lld!", id);
        return extent_protocol::NOENT;
    }
    
    auto file_it = ext_it->second;
    file_it->buf = buf;
    file_it->attr.ctime = file_it->attr.mtime = std::time(nullptr);
    file_it->attr.size = file_it->buf.size();

    return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  // You fill this in for Lab 2.
    printf("===================GET=========================================\n");
    if(!isfile(id)){
        printf("The extent with id:%lld is directory, not file!\n", id);
        return extent_protocol::NOENT;
    }

    ScopedLock gl(&_m);
    
    auto ext_it = ext_map.find(id);
    if(ext_it == ext_map.end()){
        printf("No file exists with id:%lld!\n", id);
        return extent_protocol::NOENT;
    }
    
    auto file_it = ext_it->second;
    buf = file_it->buf;
    file_it->attr.atime = std::time(nullptr);

    return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  // You fill this in for Lab 2.
    printf("===================GETATTR=====================================\n");
    ScopedLock ml(&_m);
    auto ext_it = ext_map.find(id);
    if(ext_it == ext_map.end()){
        printf("No file exists with id:%lld!\n", id);
        return extent_protocol::NOENT;
    }
    
    a.size = ext_it->second->attr.size;
    a.atime = ext_it->second->attr.atime;
    a.mtime = ext_it->second->attr.mtime;
    a.ctime = ext_it->second->attr.ctime;
    return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  // You fill this in for Lab 2.
    printf("===================REMOVE======================================\n");
    ScopedLock ml(&_m);
    
    auto ext_it = ext_map.find(id);
    if(ext_map.find(id) == ext_map.end()){
        printf("No file exists with id:%lld!\n", id);
        return extent_protocol::NOENT;
    }

    auto file_it = ext_it->second;
    extent_protocol::extentid_t parent_eid = file_it->parent_eid;
    std::string name = file_it->name;
   
    auto dir_it = ext_map.find(parent_eid);
    if(dir_it == ext_map.end()){
        printf("The file with id:%lld reside in a nonexisted dir with id:%lld\n", id, parent_eid);
        ext_map.erase(ext_it);
        return extent_protocol::NOENT;
    } 
    
    auto file_entry = dir_it->second->file_entry;
    auto file_entry_it = file_entry.find(name);
    if(file_entry_it == file_entry.end()){
        printf("No file exists with id:%lld in dir:%lld!\n", id, dir_it->second->eid);
        ext_map.erase(ext_it);
        return extent_protocol::NOENT;
    }

    file_entry.erase(file_entry_it);
    dir_it->second->attr.mtime = dir_it->second->attr.ctime = std::time(nullptr);

    ext_map.erase(ext_it);

    return extent_protocol::OK;
}

int extent_server::lookup(extent_protocol::extentid_t parent_eid, std::string name, extent_protocol::extentid_t &ret)
{
    printf("===================LOOKUP======================================\n");
    ScopedLock lul(&_m);

    auto ext_it = ext_map.find(parent_eid);
    if(ext_it == ext_map.end()){
        printf("No dir exists with id:%lld\n!", parent_eid);
        return extent_protocol::NOENT;
    }
    
    auto dir_it = ext_it->second;
    auto file_entry_it = dir_it->file_entry.find(name);
    if(file_entry_it == dir_it->file_entry.end()){
        printf("No file with name:%s exists in dir with dir_id:%lld!\n", name.c_str(), parent_eid);
        return extent_protocol::NOENT;
    }

    ret = file_entry_it->second;
    return extent_protocol::OK;
}

int extent_server::create(extent_protocol::extentid_t parent_eid, std::string name, extent_protocol::extentid_t &ret)
{
    printf("===================CREATE======================================\n");
    if(isfile(parent_eid)){
        printf("The extent with id:%lld is a file, not a dir!\n", parent_eid);
        return extent_protocol::IOERR;
    }

    ScopedLock cl(&_m);
    auto ext_it = ext_map.find(parent_eid);
    if(ext_it == ext_map.end()){
        printf("The directory with id:%lld is not existed!\n", parent_eid);
        return extent_protocol::NOENT;
    }

    auto dir_it = ext_it->second;
    auto file_entry_it = dir_it->file_entry.find(name);
    if(file_entry_it != dir_it->file_entry.end()){
        printf("The file with the name:%s is existed in the dir with id:%lld\n", name.c_str(), parent_eid);
        return extent_protocol::EXIST;
    }

    // pick a random inum for the new extent
    for(int i = 0; i < 10; ++i){
        extent_protocol::extentid_t new_inum = rd_gen();
        new_inum |= 0x80000000;

        if(ext_map.find(new_inum) == ext_map.end()){
            ext_map[new_inum] = std::make_shared<extent_t> (new_inum, parent_eid, name, 0);
            ext_map[new_inum]->file_entry[name] = new_inum;
            dir_it->file_entry[name] = new_inum;
            ret = new_inum;
            dir_it->attr.ctime = dir_it->attr.mtime = std::time(nullptr);
            return extent_protocol::OK;
        }
    }

    return extent_protocol::IOERR;
}

int extent_server::readdir(extent_protocol::extentid_t id, std::map<std::string, extent_protocol::extentid_t> &ret)
{
    printf("===================READDIR=====================================\n");
    ScopedLock rdl(&_m);
    auto ext_it = ext_map.find(id);
    if(ext_it == ext_map.end()){
        printf("No dir exists with id:%lld!\n", id);
        return extent_protocol::NOENT;
    }

    auto dir_it = ext_it->second;
    for(auto &e_it : dir_it->file_entry){
        ret[e_it.first] = e_it.second;
    }

    dir_it->attr.atime = std::time(nullptr);
    return extent_protocol::OK;
}
