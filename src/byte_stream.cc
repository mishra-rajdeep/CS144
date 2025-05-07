#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity ), pushed( 0 ), popped( 0 ), closed( false ), str( "" )
{}

void Writer::push( string data )
{
  uint64_t i = 0;
  while ( str.size() < capacity_ && i < data.size() ) {
    str += data[i++];
    pushed++;
  }
}

void Writer::close()
{
  closed = true;
}

bool Writer::is_closed() const
{
  return closed;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - str.size();
}

uint64_t Writer::bytes_pushed() const
{
  return pushed;
}

string_view Reader::peek() const
{
  return string_view( str );
}

void Reader::pop( uint64_t len )
{
  popped += min( len, str.length() );
  str = str.substr( min( len, str.length() ) );
}

bool Reader::is_finished() const
{
  return ( str.size() == 0 && closed );
}

uint64_t Reader::bytes_buffered() const
{
  return str.size();
}

uint64_t Reader::bytes_popped() const
{
  return popped;
}
