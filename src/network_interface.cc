#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t dst_ip = next_hop.ipv4_numeric();
  if(mapping.count(dst_ip)) {
    EthernetFrame frame;
    frame.payload = serialize(dgram);
    frame.header.src = ethernet_address_;
    frame.header.dst = mapping[dst_ip];
    frame.header.type = EthernetHeader::TYPE_IPv4;
    sending_queue.push(frame);
  }else {
    if(waiting_arp.count(dst_ip) ) return;
    EthernetFrame frame;
    ARPMessage arp_message;
    arp_message.opcode = ARPMessage::OPCODE_REQUEST;
    arp_message.sender_ethernet_address = ethernet_address_;
    arp_message.sender_ip_address = ip_address_.ipv4_numeric();
    arp_message.target_ip_address = next_hop.ipv4_numeric();
    frame.payload = serialize(arp_message);
    frame.header.src = ethernet_address_;
    frame.header.dst = ETHERNET_BROADCAST;
    frame.header.type = EthernetHeader::TYPE_ARP;
    sending_queue.push(frame);
    waiting_arp.insert({dst_ip,dgram});
  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if(frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST) return {};
  if(frame.header.type == EthernetHeader::TYPE_IPv4) {
    IPv4Datagram datagram;
    if(parse(datagram,frame.payload)) return datagram;
  }else {
    ARPMessage arp_message;
    if(!parse(arp_message,frame.payload) ) return {};
    uint32_t src_ip = arp_message.sender_ip_address;
    EthernetAddress src_adr = arp_message.sender_ethernet_address;
    if(!mapping.count(src_ip)) {
      mapping.insert({src_ip,src_adr});
      mapping_timer.insert({src_ip,0});
      if(waiting_arp.count(src_ip)) {
        EthernetFrame frame2;
        frame2.payload = serialize(waiting_arp[src_ip]);
        frame2.header.src = ethernet_address_;
        frame2.header.dst = src_adr;
        frame2.header.type = EthernetHeader::TYPE_IPv4;
        sending_queue.push(frame2);
        waiting_arp.erase(src_ip);
      }
    }
    // send arp repay message
    if(arp_message.opcode == ARPMessage::OPCODE_REQUEST && arp_message.target_ip_address == ip_address_.ipv4_numeric()) {
      EthernetFrame sending_frame;
      ARPMessage arp_repay_msg;
      arp_repay_msg.opcode = ARPMessage::OPCODE_REPLY;
      arp_repay_msg.sender_ethernet_address = ethernet_address_;
      arp_repay_msg.sender_ip_address = ip_address_.ipv4_numeric();
      arp_repay_msg.target_ethernet_address = src_adr;
      arp_repay_msg.target_ip_address = src_ip;
      sending_frame.payload = serialize(arp_repay_msg);
      sending_frame.header.src = ethernet_address_;
      sending_frame.header.dst = src_adr;
      sending_frame.header.type = EthernetHeader::TYPE_ARP;
      sending_queue.push(sending_frame);
    }
  }
  return {};
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  arp_ms_passed += ms_since_last_tick;
  if(arp_ms_passed > ARP_WAITINT_MS) {
    arp_ms_passed = 0;
    waiting_arp.clear();
  }
  vector<uint32_t> key_to_remove;
  for(auto &kv:mapping_timer) {
    kv.second += ms_since_last_tick;
    if(kv.second > MAPPING_TIMEOUT_MS) {
      key_to_remove.push_back(kv.first);
    }
  }
  for(auto key: key_to_remove) {
    mapping.erase(key);
    mapping_timer.erase(key);
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if(sending_queue.empty() ) return {};
  auto x = sending_queue.front();
  sending_queue.pop();
  return x;
}

Address NetworkInterface::get_ip_address() {
  return ip_address_;
}