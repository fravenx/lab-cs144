#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  if(!hasisn) {
    if(!message.SYN) return;
    isn = message.seqno;
    hasisn = true;
  }
  uint64_t abs_ackno = reassembler.next_byte_();
  uint64_t first_index = message.seqno.unwrap(isn,abs_ackno) - 1 + message.SYN;
  reassembler.insert(first_index,message.payload,message.FIN,inbound_stream);
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  unsigned short window_size = inbound_stream.available_capacity() > UINT16_MAX? UINT16_MAX:inbound_stream.available_capacity();
  uint64_t abs_ackno = inbound_stream.bytes_pushed() + 1 + inbound_stream.is_closed();
  Wrap32 ackno = Wrap32(0).wrap(abs_ackno,isn);
  if(!hasisn) return TCPReceiverMessage{std::optional<Wrap32>{}, window_size};
  return TCPReceiverMessage{ackno, window_size};

}
