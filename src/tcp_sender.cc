#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), 
    initial_RTO_ms_( initial_RTO_ms ),
    RTO_ms_( initial_RTO_ms )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return outstanding_size;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{ 
  if(segmentsout_queue.empty()) return {};
  auto res = segmentsout_queue.front();
  segmentsout_queue.pop();
  return res;
}

void TCPSender::sendTCPMessage(TCPSenderMessage m) {
  m.seqno = Wrap32::wrap(next_byte_tosend,isn_);
  outstanding_queue.push(m);
  segmentsout_queue.push(m);
  outstanding_size += m.sequence_length();
  next_byte_tosend += m.sequence_length();
  if(!RTO_running) {
    RTO_running = true;
    RTO_timer = 0;
  }
}

void TCPSender::push( Reader& outbound_stream )
{ 
  if(fin_) return;
  TCPSenderMessage message = TCPSenderMessage();
  uint64_t win_size = window_size > 0? window_size:1;
  if(!syn_) {
    message.SYN = true;
    if(outbound_stream.is_finished() && abs_ackno + win_size > next_byte_tosend) message.FIN = true,fin_ = true;
    sendTCPMessage(message);
    syn_ = true;
    return;
  }
  if(outbound_stream.is_finished() && abs_ackno + win_size > next_byte_tosend  && !fin_) {
    message.FIN = true;
    sendTCPMessage(message);
    fin_ = true;
    return; 
  }
  // outstanding_size = next_byte_tosend - abs_ackno
  while(outbound_stream.bytes_buffered() && abs_ackno + win_size > next_byte_tosend) {
    uint64_t size = min(TCPConfig::MAX_PAYLOAD_SIZE,win_size - outstanding_size);
    message.payload = outbound_stream.pop2(min(size, outbound_stream.bytes_buffered()));
    if(outbound_stream.is_finished() && next_byte_tosend + message.sequence_length() < win_size + abs_ackno) message.FIN = true,fin_ = true;
    sendTCPMessage(message);
  }

}

TCPSenderMessage TCPSender::send_empty_message() 
{
  TCPSenderMessage m = TCPSenderMessage();
  m.seqno = Wrap32::wrap(next_byte_tosend,isn_);
  return m;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{ 
  if(!msg.ackno.has_value()) {
    window_size = msg.window_size;
    return;
  }
  uint64_t abs_ackno_ = msg.ackno.value().unwrap(isn_,next_byte_tosend);
  if(abs_ackno_ > next_byte_tosend || !syn_) return;
  if(abs_ackno_ >= abs_ackno) {
    abs_ackno = abs_ackno_;
    window_size = msg.window_size;
  }
  while(outstanding_queue.size()) {
    TCPSenderMessage m = outstanding_queue.front();
    if(m.seqno.unwrap(isn_,next_byte_tosend) + m.sequence_length() > abs_ackno_) break;
    outstanding_queue.pop();
    outstanding_size -= m.sequence_length();
    RTO_ms_ = initial_RTO_ms_;
    RTO_timer = 0;
    consecutive_retransmissions_ = 0;
  }
  if(outstanding_queue.size() == 0) RTO_running = false;
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  if(!RTO_running) return;
  RTO_timer += ms_since_last_tick;
  if(RTO_timer >= RTO_ms_) {
    TCPSenderMessage msg = outstanding_queue.front();
    segmentsout_queue.push(msg);
    RTO_timer = 0;
    if(window_size > 0 || msg.SYN) {
      RTO_ms_ *= 2;
      consecutive_retransmissions_++;
    }
  }
}
