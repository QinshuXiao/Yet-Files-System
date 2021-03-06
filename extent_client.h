// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include <map>
#include "extent_protocol.h"
#include "rpc.h"

class extent_client {
 private:
  rpcc *cl;

 public:
  extent_client(std::string dst);

  extent_protocol::status get(extent_protocol::extentid_t eid, 
			      std::string &buf);
  
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				  extent_protocol::attr &attr);
  
  extent_protocol::status put(extent_protocol::extentid_t eid, 
                  std::string buf);
  
  extent_protocol::status remove(extent_protocol::extentid_t eid);

  extent_protocol::status create(extent_protocol::extentid_t parent_eid, 
                  std::string name, extent_protocol::extentid_t eid);
  
  extent_protocol::status lookup(extent_protocol::extentid_t parent_eid, 
                  std::string name, extent_protocol::extentid_t &eid);
  
  extent_protocol::status readdir(extent_protocol::extentid_t eid, 
                  std::map<std::string, extent_protocol::extentid_t> &ext_map);
};

#endif 

