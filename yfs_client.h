#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>
#include <map>
#include <utility>

#include "lock_protocol"
#include "lock_client.h"

class yfs_client {
  extent_client *ec;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

 private:
  static std::string filename(inum);
  static inum n2i(std::string);
 public:

  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int setfile(inum, struct stat *);
  int getdir(inum, dirinfo &);
  int readfile(inum, std::size_t, std::size_t, std::string &);
  int writefile(inum, std::size_t, std::size_t, const char *);

  int create(inum, std::string, extent_protocol::extentid_t &);
  int lookup(inum, std::string, extent_protocol::extentid_t &);
  int readdir(inum, std::map<std::string, extent_protocol::extentid_t> &);
  int unlink(inum, const char*);
};

#endif 
