#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"
#include <iostream>
using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return next_seqno - last_ackno;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  return cons_retrans;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  if ( window_size <= 0 ) {
    if ( !sent_syn ) {
      auto msg = TCPSenderMessage();
      msg.SYN = true;
      msg.seqno = isn_;
      transmit( msg );
      track.push( next_seqno, msg );
      sent_syn = true;
      timer = curr_RTO;
      next_seqno = 1;
    } else if ( window_size_receiver == 0 && !zero_sent_once ) {
      // transmit( make_empty_message() );
      if ( input_.reader().bytes_buffered() ) {
        TCPSenderMessage msg;
        msg.seqno = Wrap32::wrap( next_seqno, isn_ );
        auto buf = input_.reader().peek();
        msg.payload = std::string( buf.data(), 1 );
        msg.RST = input_.reader().has_error() || err;
        input_.reader().pop( 1 );
        transmit( msg );
        track.push( next_seqno, msg );
        // if ( track.size() == 1 )
        //   timer = curr_RTO;
        next_seqno += msg.sequence_length();
      } else if ( input_.reader().is_finished() && !sent_fin ) {
        TCPSenderMessage msg;
        msg.FIN = true;
        msg.RST = input_.reader().has_error() || err;
        msg.seqno = Wrap32::wrap( next_seqno, isn_ );
        transmit( msg );
        track.push( next_seqno, msg );
        // if ( track.size() == 1 )
        //   timer = curr_RTO;
        sent_fin = true;
        next_seqno += msg.sequence_length();
      }
      zero_sent_once = true;

      // window_size_receiver = -101;
    }
    return;
  }
  while ( window_size > 0 ) {
    if ( input_.reader().bytes_buffered() ) {
      TCPSenderMessage msg;
      msg.RST = input_.reader().has_error() || err;
      msg.seqno = Wrap32::wrap( next_seqno, isn_ );
      uint64_t read_len = window_size;
      if ( !sent_syn ) {
        msg.SYN = true;
        sent_syn = true;
        timer = curr_RTO;
        read_len--;
      }
      if ( input_.reader().bytes_buffered() < read_len )
        read_len = input_.reader().bytes_buffered();
      if ( read_len > TCPConfig::MAX_PAYLOAD_SIZE )
        read_len = TCPConfig::MAX_PAYLOAD_SIZE;
      read( input_.reader(), read_len, msg.payload );
      window_size -= msg.sequence_length();
      if ( input_.reader().is_finished() && window_size > 0 ) {
        msg.FIN = true;
        sent_fin = true;
        window_size--;
      }
      transmit( msg );
      track.push( next_seqno, msg );
      if ( track.size() == 1 )
        timer = curr_RTO;
      next_seqno += msg.sequence_length();
    } else if ( input_.reader().is_finished() && !sent_fin ) {
      TCPSenderMessage msg;
      if ( !sent_syn ) {
        msg.SYN = true;
        sent_syn = true;
        timer = curr_RTO;
      }
      msg.RST = input_.reader().has_error() || err;
      msg.FIN = true;
      msg.seqno = Wrap32::wrap( next_seqno, isn_ );
      transmit( msg );
      track.push( next_seqno, msg );
      if ( track.size() == 1 )
        timer = curr_RTO;
      sent_fin = true;
      window_size -= msg.sequence_length();
      next_seqno += msg.sequence_length();
    } else
      break;
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  auto msg = TCPSenderMessage();
  msg.seqno = Wrap32::wrap( next_seqno, isn_ );
  msg.RST = input_.reader().has_error() || err;
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  err = msg.RST;
  if ( err )
    input_.reader().set_error();
  if ( !msg.ackno.has_value() ) {
    window_size = msg.window_size;
    window_size_receiver = msg.window_size;
    if ( window_size_receiver == 0 )
      zero_sent_once = false;
    return;
  }
  uint32_t ack = msg.ackno.value().unwrap( isn_, last_ackno );
  if ( ack >= last_ackno && ack <= next_seqno ) {
    if ( ack > last_ackno ) {
      last_ackno = ack;
      curr_RTO = initial_RTO_ms_;
      timer = curr_RTO;
      cons_retrans = 0;
    }
    window_size = ack + msg.window_size - next_seqno;
    window_size_receiver = msg.window_size;
    if ( window_size_receiver == 0 )
      zero_sent_once = false;
    track.update( last_ackno );
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  timer -= ms_since_last_tick;
  if ( timer <= 0 && track.hasMsg() ) {
    auto msg = track.getMsg();
    transmit( msg );
    cons_retrans++;
    if ( window_size_receiver != 0 )
      curr_RTO *= 2;
    timer = curr_RTO;
  }
}

TCPSenderMessage AckTrack::getMsg()
{
  TCPSenderMessage msg = dq.front().second;
  return msg;
}
void AckTrack::update( uint32_t ackno )
{
  while ( !dq.empty() && ( dq.front().first + dq.front().second.sequence_length() <= ackno ) )
    dq.pop_front();
}
uint64_t AckTrack::size() const
{
  return dq.size();
}
void AckTrack::push( uint32_t seqno, const TCPSenderMessage& msg )
{
  dq.emplace_back( seqno, msg );
}
bool AckTrack::hasMsg() const
{
  return !dq.empty();
}
