#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include "queue"

class TCPSender
{
private:
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t next_byte_tosend = 0;
  uint64_t abs_ackno = 0;
  uint64_t window_size = 1;
  uint64_t outstanding_size = 0;
  uint64_t RTO_ms_;
  uint64_t RTO_timer = 0;
  bool syn_ = false;
  bool fin_ = false;
  bool RTO_running = false;
  std::queue<TCPSenderMessage> outstanding_queue = {};
  std::queue<TCPSenderMessage> segmentsout_queue = {};
  uint64_t consecutive_retransmissions_ = 0;

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() ;

  void sendTCPMessage(TCPSenderMessage m);

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
