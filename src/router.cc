#include "router.hh"
#include "debug.hh"

#include <iostream>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  routing_table.emplace_back( route_prefix, prefix_length, next_hop, interface_num );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for ( auto& interface : interfaces_ ) {
    auto& q = interface->datagrams_received();
    while ( !q.empty() ) {
      auto dgram = q.front();
      q.pop();
      if ( dgram.header.ttl <= 1 )
        continue;
      dgram.header.ttl--;
      dgram.header.compute_checksum();
      uint8_t best_len = 0;
      std::optional<Address> best_next_hop;
      size_t best_interface = -1;
      for ( auto& [route_prefix, prefix_length, next_hop, interface_num] : routing_table ) {
        uint32_t mask = ( prefix_length == 0 ) ? 0 : ( ~uint32_t( 0 ) << ( 32 - prefix_length ) );
        if ( ( dgram.header.dst & mask ) == ( route_prefix & mask ) ) {
          if ( best_interface == size_t( -1 ) || prefix_length > best_len ) {
            best_len = prefix_length;
            best_next_hop = next_hop;
            best_interface = interface_num;
          }
        }
      }
      if ( best_interface != size_t( -1 ) ) {
        Address target
          = best_next_hop.has_value() ? best_next_hop.value() : Address::from_ipv4_numeric( dgram.header.dst );
        interfaces_.at( best_interface )->send_datagram( dgram, target );
      }
    }
  }
}
