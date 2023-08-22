#include "reassembler.hh"
#include "iostream"
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{ 
  if(first_index >= next_byte + output.available_capacity()) return;

  if(is_last_substring) eof = true;

  if(first_index + data.size() > next_byte) {
    for(uint64_t i = max(next_byte,first_index);i < first_index + data.size() && i < next_byte + output.available_capacity();i++) {
    if(!hash.count(i)) hash.insert({i,data[i - first_index]});
    }

    while(hash.count(next_byte)) {
      output.push(string(1,hash[next_byte]));
      hash.erase(next_byte);
      next_byte++;
    }
    bytes_pending_ = hash.size();
  }
  
  if(eof && hash.empty()) output.close();
}

uint64_t Reassembler::bytes_pending() const
{
  return bytes_pending_;
}
