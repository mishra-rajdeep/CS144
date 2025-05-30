#include "tcp_receiver.hh"
#include "debug.hh"
#include <iostream>
using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.SYN ) {
    syn = true;
    zero_point = message.seqno;
    checkpoint = 0;
  }
  if ( message.RST )
    reassembler_.reader().set_error();
  if ( !syn )
    return;
  reassembler_.insert( message.seqno.unwrap( zero_point, reassembler_.writer().bytes_pushed() ) - 1 + message.SYN,
                       message.payload,
                       message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  auto msg = TCPReceiverMessage();
  if ( syn )
    msg.ackno
      = Wrap32::wrap( reassembler_.writer().bytes_pushed() + 1 + reassembler_.writer().is_closed(), zero_point );
  msg.RST = reassembler_.writer().has_error();
  msg.window_size = min( reassembler_.writer().available_capacity(), (uint64_t)65535 );
  return msg;
}
