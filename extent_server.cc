// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extent_server::extent_server() {
    VERIFY(pthread_mutex_init(&_m, 0) == 0);
    ext_map[1] = std::make_shared<extent_t>(1,0,"",0);
}

extent_server::~extent_server() {
    VERIFY(pthread_mutex_destroy(&_m) == 0);
}


int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
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
    file_it->attr.mtime = std::time(nullptr);
    file_it->attr.size = file_it->buf.size();

    printf("  Succeed put operation on %lld\n",id);
    return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
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

    printf("  Succeed get operation on %lld\n",id);
    return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
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

    printf("  Succeed getattr operation on %lld\n",id);
    return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
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
   
    auto p_it = ext_map.find(parent_eid);
    if(p_it == ext_map.end()){
        printf("The file with id:%lld reside in a nonexisted dir with id:%lld\n", id, parent_eid);
        ext_map.erase(ext_it);
        return extent_protocol::NOENT;
    } 
    
    auto dir_ext = p_it->second;
    if(dir_ext->file_entry.find(name) == dir_ext->file_entry.end()){
        printf("No file exists with id:%lld in dir:%lld!\n", id, dir_ext->eid);
        ext_map.erase(ext_it);
        return extent_protocol::NOENT;
    }

    dir_ext->file_entry.erase(name);
    dir_ext->attr.mtime = p_it->second->attr.ctime = std::time(nullptr);

    ext_map.erase(ext_it);

    printf("  Succeed remove operation on %lld under dir:%lld\n",id, parent_eid);
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

    printf("  Succeed lookuo operation on %s under dir:%lld\n",name.c_str(), parent_eid);
    return extent_protocol::OK;
}

int extent_server::create(extent_protocol::extentid_t parent_eid, std::string name, extent_protocol::extentid_t eid, int &)
{
    printf("===================CREATE======================================\n");
    if(isfile(parent_eid)){
        printf("The extent with id:%lld is a file, not a dir!\n", parent_eid);
        return extent_protocol::IOERR;
    }

    ScopedLock cl(&_m);
    auto p_it = ext_map.find(parent_eid);
    if(p_it == ext_map.end()){
        printf("THe dir with id:%lld is not existed!\n", parent_eid);
        return extent_protocol::NOENT;
    }

    auto dir_it = p_it->second;
    dir_it->file_entry[name] = eid;
    dir_it->attr.ctime = dir_it->attr.mtime = std::time(nullptr);

    ext_map[eid] = std::make_shared<extent_t> (eid, parent_eid, name, 0);
    if(isfile(eid)) ext_map[eid]->file_entry[name] = eid;
    
    printf("  Succeed create operation on %s(eid:%lld) under dir:%lld\n",name.c_str(), eid, parent_eid);
    return extent_protocol::OK;
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

    printf("  Succeed readdir operation on %lld\n",id);
    return extent_protocol::OK;
}
