#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>
#include <map>
#include <utility>
#include <random>

#include "lock_protocol.h"
#include "lock_client.h"
#include "remote_scoped_lock.h"

class yfs_client {
  extent_client *ec;
  lock_client *lc;
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
  std::default_random_engine rd_gen;
 public:

  yfs_client(std::string, std::string);
  ~yfs_client();
  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int setfile(inum, struct stat *);
  int getdir(inum, dirinfo &);
  int readfile(inum, std::size_t, std::size_t, std::string &);
  int writefile(inum, std::size_t, std::size_t, const char *);

  int create(inum, std::string, inum &, bool);
  int lookup(inum, std::string, inum &);
  int readdir(inum, std::map<std::string, inum> &);
  int unlink(inum, const char*);
};

#endif 
