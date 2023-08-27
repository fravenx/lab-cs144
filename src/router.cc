#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop_,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop_.has_value() ? next_hop_->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";
  std::pair<uint32_t, uint8_t> route_key(route_prefix, prefix_length);
  routes.push_back(route_key);
  next_hop_v.push_back(next_hop_);
  interface_num_v.push_back(interface_num);

}

void Router::route() {
  for(auto &interface : interfaces_) {
    optional<InternetDatagram> dram = interface.maybe_receive();
    if(dram.has_value()) {
      IPv4Datagram dgram = dram.value();
      if(dgram.header.ttl == 0 || dgram.header.ttl == 1) continue;
      dgram.header.ttl--;
      dgram.header.compute_checksum();
      uint32_t dst = dgram.header.dst;
      size_t longest_num = 0;
      size_t index = 0;
      optional<Address> next_hop;
      bool mapping = false;
      for(size_t i = 0;i < routes.size();i++) {
        auto r = routes[i];
        uint32_t route_prefix = r.first;
        uint8_t prefix_length = r.second;
        if(prefix_length == 0) {
          if(longest_num == 0) index = interface_num_v[i],next_hop = next_hop_v[i],mapping = true;
          else continue;
        }else if(prefix_length == 32) {
          if(route_prefix == dst) index = interface_num_v[i],next_hop = next_hop_v[i],longest_num = 32,mapping = true;
          else continue;
        }else { 
          size_t pl = prefix_length;
          size_t offset = 31;
          uint32_t prefix = 0;
          while(pl--) {
            prefix += (uint32_t(1) << offset);
            offset--; 
          }
          if((prefix & dst) == route_prefix) {
            longest_num = prefix_length;
            index = interface_num_v[i];
            next_hop = next_hop_v[i];
            mapping = true;
          }
        }
      }
      if(mapping) {
        if(!next_hop.has_value()) next_hop.emplace(Address::from_ipv4_numeric(dgram.header.dst));
        interfaces_.at(index).send_datagram(dgram,next_hop.value());
      }
    }
  }

}
