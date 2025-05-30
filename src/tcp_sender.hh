#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <deque>
#include <functional>

class AckTrack
{
public:
  AckTrack() : dq() {}
  void update( uint32_t ackno );
  TCPSenderMessage getMsg();
  uint64_t size() const;
  void push( uint32_t seqno, const TCPSenderMessage& msg );
  bool hasMsg() const;

private:
  std::deque<std::pair<uint32_t, TCPSenderMessage>> dq;
};

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) )
    , isn_( isn )
    , initial_RTO_ms_( initial_RTO_ms )
    , curr_RTO( initial_RTO_ms )
    , timer( 0 )
    , window_size( 0 )
    , cons_retrans( 0 )
    , last_ackno( 0 )
    , track()
    , sent_syn( 0 )
    , sent_fin( 0 )
    , next_seqno( 0 )
    , window_size_receiver( -101 )
    , zero_sent_once( false )
    , err( 0 )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // For testing: how many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // For testing: how many consecutive retransmissions have happened?
  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }

private:
  Reader& reader() { return input_.reader(); }

  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t curr_RTO;
  int64_t timer;
  uint64_t window_size;
  uint64_t cons_retrans;
  uint32_t last_ackno;
  AckTrack track;
  bool sent_syn;
  bool sent_fin;
  uint32_t next_seqno;
  uint64_t window_size_receiver;
  bool zero_sent_once;
  bool err;
};
