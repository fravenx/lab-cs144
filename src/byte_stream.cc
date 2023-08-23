#include <stdexcept>
#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity_ ): capacity(capacity_) , stream(queue<char>()) , error(false) , 
  closed(false) , bytes_pushed_(0) , bytes_buffered_(0) , bytes_popped_(0) , str(""){
  }

void Writer::push( string data )
{
  uint64_t avail = capacity - stream.size();
  uint64_t length = min(avail,data.size());
  for(uint64_t i = 0,j = 0;j < length;i++,j++) {
    stream.push(data[i]);
  }
  if(length > 0) str += data.substr(0, length);
  bytes_pushed_ += length;
}

void Writer::close()
{
  closed = true;
}

void Writer::set_error()
{
  error = true;
}

bool Writer::is_closed() const
{
  return closed;
}

uint64_t Writer::available_capacity() const
{
  return capacity - stream.size();
}

uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_;
}

string_view Reader::peek() const
{
  return str;
}

bool Reader::is_finished() const
{
  return closed && bytes_popped_ == bytes_pushed_;
}

bool Reader::has_error() const
{
  return error;
}

void Reader::pop( uint64_t len )
{
  for(uint64_t i = 0;i < len;i++) {
    stream.pop();
  }
  str = str.substr(len);
  bytes_popped_ += len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return bytes_pushed_ - bytes_popped_;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}
