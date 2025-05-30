#include <iostream>

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"
using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
  , curr_time_ms( 0 )
  , arp_map()
  , arp_time()
  , wait_queue()
{}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t ip_addr = next_hop.ipv4_numeric();
  if ( arp_map.find( ip_addr ) != arp_map.end() && curr_time_ms - arp_map[ip_addr].first <= 30000 ) {
    __ip_send( dgram, ip_addr );
  } else {
    if ( arp_time.find( ip_addr ) == arp_time.end() || curr_time_ms - arp_time[ip_addr] > 5000 ) {
      __arp_request( ip_addr );
      arp_time[ip_addr] = curr_time_ms;
    }
    wait_queue[ip_addr].push_back( { curr_time_ms, dgram } );
  }
}
// send ARP request
void NetworkInterface::__arp_request( uint32_t ip_addr )
{
  EthernetFrame frame;
  EthernetHeader eth_hdr;
  eth_hdr.dst = ETHERNET_BROADCAST;
  eth_hdr.src = ethernet_address_;
  eth_hdr.type = EthernetHeader::TYPE_ARP;
  frame.header = eth_hdr;
  ARPMessage arp;
  arp.opcode = ARPMessage::OPCODE_REQUEST;
  arp.sender_ethernet_address = ethernet_address_;
  arp.sender_ip_address = ip_address_.ipv4_numeric();
  arp.target_ethernet_address = { 0, 0, 0, 0, 0, 0 };
  arp.target_ip_address = ip_addr;
  Serializer ser;
  arp.serialize( ser );
  frame.payload = ser.finish();
  transmit( frame );
}
// send datagram to ip addr
void NetworkInterface::__ip_send( InternetDatagram dgram, uint32_t ip_addr )
{
  EthernetFrame msg;
  EthernetHeader header;
  header.type = EthernetHeader::TYPE_IPv4;
  header.src = ethernet_address_;
  header.dst = arp_map[ip_addr].second;
  msg.header = header;
  Serializer ser = Serializer();
  dgram.serialize( ser );
  msg.payload = ser.finish();
  transmit( msg );
}
// send ARP reply
void NetworkInterface::__arp_reply( ARPMessage& arpmsg )
{
  ARPMessage arp_reply;
  arp_reply.opcode = arpmsg.OPCODE_REPLY;
  arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
  arp_reply.sender_ethernet_address = ethernet_address_;
  arp_reply.target_ip_address = arpmsg.sender_ip_address;
  arp_reply.target_ethernet_address = arpmsg.sender_ethernet_address;

  EthernetFrame frame_out;
  EthernetHeader eth;
  eth.src = ethernet_address_;
  eth.dst = arpmsg.sender_ethernet_address;
  eth.type = EthernetHeader::TYPE_ARP;
  frame_out.header = eth;

  Serializer ser;
  arp_reply.serialize( ser );
  frame_out.payload = ser.finish();

  transmit( frame_out );
}
//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return;
  }
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram dgram;
    if ( parse( dgram, frame.payload ) ) {
      datagrams_received_.push( dgram );
    }
  } else if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arpmsg;
    if ( parse( arpmsg, frame.payload ) ) {
      arp_map[arpmsg.sender_ip_address] = std::make_pair( curr_time_ms, arpmsg.sender_ethernet_address );
      if ( arpmsg.OPCODE_REQUEST == arpmsg.opcode && arpmsg.target_ip_address == ip_address_.ipv4_numeric() ) {
        __arp_reply( arpmsg );
      }
      while ( !wait_queue[arpmsg.sender_ip_address].empty() ) {
        auto msg = wait_queue[arpmsg.sender_ip_address].front();
        wait_queue[arpmsg.sender_ip_address].pop_front();
        if ( curr_time_ms - msg.first > 5000 )
          continue;
        __ip_send( msg.second, arpmsg.sender_ip_address );
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  curr_time_ms += ms_since_last_tick;
}
