#include "reassembler.hh"
#include "debug.hh"
#include <iostream>
#include <string>
using namespace std;

void Reassembler::insert( uint64_t index, string data, bool is_last_substring )
{
  if ( is_last_substring ) {
    end = index + data.size();
  }

  // Clip data to fit into current window
  uint64_t window_end = next + output_.writer().available_capacity();
  uint64_t data_start = std::max( index, next );
  uint64_t data_end = std::min( index + data.size(), window_end );

  for ( uint64_t i = data_start; i < data_end; ++i ) {
    if ( !temp.count( i ) ) {
      temp[i] = data[i - index];
    }
  }

  string s;
  while ( temp.count( next ) ) {
    s += temp[next];
    temp.erase( next );
    // del++;   // speedup but feels like cheating
    ++next;
  }
  output_.writer().push( s );

  if ( next == end ) {
    output_.writer().close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  return temp.size();
}
