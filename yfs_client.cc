// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>


yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);

}

yfs_client::inum yfs_client::n2i(std::string n)
{
  std::istringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

std::string yfs_client::filename(inum inum)
{
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}

bool yfs_client::isfile(inum inum)
{
  if(inum & 0x80000000)
    return true;
  return false;
}

bool yfs_client::isdir(inum inum)
{
  return ! isfile(inum);
}

int yfs_client::getfile(inum inum, fileinfo &fin)
{
  int r = OK;

  printf("get file attr %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    return r;
  }

  fin.atime = a.atime;
  fin.mtime = a.mtime;
  fin.ctime = a.ctime;
  fin.size = a.size;
  printf("getfile %016llx -> sz %llu\n", inum, fin.size);

  return r;
}

int yfs_client::setfile(inum inum, struct stat *attr)
{
    int r = OK;

    printf("set file attr %016llx\n", inum);

    std::string content;

    extent_protocol::status ret = ec->get(inum, content);
    if(ret != extent_protocol::OK){
        if(ret == extent_protocol::NOENT){
            printf("ERROR: No such file %016llx exists !", inum);
            r = NOENT;
        }
        else{
            r = IOERR;
        }
        return r;
    }

    if(attr->st_size < (int)content.size()){
        content = content.substr(0, attr->st_size);
    }
    else{
        content += std::string(attr->st_size - content.size(), '\0');
    }

    ret = ec->put(inum, content);
    if(ret != extent_protocol::OK){
        r = IOERR;
        return r;
    }
    printf("setfile %016llx -> sz %ld\n", inum, attr->st_size);
    return r;
}

int yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;

  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    return r;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

  return r;
}

int yfs_client::readfile(inum inum, std::size_t off, std::size_t size, std::string &buf){
    int r = OK;

    printf("Read file with id: %016llx\n", inum);
    if(!isfile(inum)){
        r = NOENT;
        printf("Error: Such inum: %016llx is unvalid for a file!\n", inum);
        return r;
    }
    
    std::string tmp;
    extent_protocol::status ret = ec->get(inum, tmp);
    if(ret != extent_protocol::OK){
        if(ret == extent_protocol::NOENT){
            printf("Error: Read file fail! No such file with id:%016llx\n", inum);
            r = NOENT;
            return r;
        }
        else{
            r = IOERR;
            return r;
        }
    }
    
    if(off < 0){
        printf("ERROR: invaild off value(%ld)! Smaller than 0!\n", off);
    }

    buf = tmp.substr(off, size);
    return r;
}

int yfs_client::writefile(inum inum, std::size_t off, std::size_t size, const char *buf){
    int r = OK;

    printf("Write file with id:%016llx!\n", inum);
    if(!isfile(inum)){
        r = NOENT;
        printf("Error: Such inum:%016llx is unvalid for file!\n", inum);
        return r;
    }

    std::string content;
    extent_protocol::status ret = ec->get(inum, content);
    if(ret != extent_protocol::OK){
        if(ret == extent_protocol::NOENT){
            printf("Error: Read file fail! No such file with id:%016llx\n", inum);
            r = NOENT;
            return r;
        }
        else{
            r = IOERR;
            return r;
        }
    }
    
    if(off < 0){
        printf("ERROR: invalid Off value(%ld)! smaller than 0!\n", off);
        r = IOERR;
        return r;
    }

    content.resize(std::max(content.size(), off + size), '\0');

    for(size_t i = off, j = 0; j < size; ++i, ++j){
        content[i] = buf[j];
    }

    ret = ec->put(inum, content);
    if(ret != extent_protocol::OK){
        if(ret == extent_protocol::NOENT){
            printf("Error: Write file failed! No such file with id:%016llx\n", inum);
            r = NOENT;
        }
        else{
            r = IOERR;
        }
    }
    
    return r;
}

int yfs_client::create(inum pid, std::string name, extent_protocol::extentid_t &eid)
{
    int r = OK;

    printf("Create a file named:%s under the dir with id:%lld!\n", name.c_str(), pid);

    if(isfile(pid)){
        r = IOERR;
        printf("The inum of dir (id:%lld) is invaild, because it's a inum id for file!\n", pid);
        return r;
    }

    extent_protocol::status ret = ec->create(pid, name, eid);
    if(ret != extent_protocol::OK){
        if(ret == extent_protocol::EXIST){
            printf("The file with name:%s is existed under the dir:%lld!\n", name.c_str(), pid);
            r = EXIST;
        }
        else if(ret == extent_protocol::IOERR){
            printf("IO ERROR happened when creating a new file(name:%s) under the dir:%lld!\n", name.c_str(), pid);
            r = IOERR;
        }
        else if(ret == extent_protocol::NOENT){
            printf("No dir with id:%lld exists in ext_server!\n", pid);
            r = NOENT;
        }
    }

    return r;
}

int yfs_client::lookup(inum pid, std::string name, extent_protocol::extentid_t &eid)
{
    int r = OK;
    printf("Looking for a file named:%s under the dir:%lld\n", name.c_str(), pid);

    if(isfile(pid)){
        r = IOERR;
        printf("The inum of dir (id:%lld) is invalid, because it's a inum id for file!\n", pid);
        return r;
    }

    extent_protocol::status ret;
    ret = ec->lookup(pid, name, eid);
    if(ret == extent_protocol::NOENT){
        r = NOENT;
        printf("No file named:%s exists under the dir:%lld\n", name.c_str(), pid);
    }

    return r;
}

int yfs_client::readdir(inum dir_id, std::map<std::string, extent_protocol::extentid_t> &ext_map)
{
    int r = OK;
    printf("Reading dir info (dir id:%lld)!\n", dir_id);

    extent_protocol::status ret = ec->readdir(dir_id, ext_map);
    if(ret == extent_protocol::NOENT){
        r = NOENT;
        printf("No dir(id:%lld) exists!\n", dir_id);
    }

    return r;
}
