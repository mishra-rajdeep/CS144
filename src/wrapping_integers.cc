#include "wrapping_integers.hh"
#include "debug.hh"
#include <iostream>
using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + uint32_t( n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint32_t curr = raw_value_ - zero_point.raw_value_;
  uint64_t a = ( checkpoint - curr ) / ( (uint64_t)1 << 32 );
  uint64_t first = (uint64_t)curr + ( (uint64_t)1 << 32 ) * a;
  uint64_t second = (uint64_t)curr + ( (uint64_t)1 << 32 ) * ( a + 1 );
  uint64_t diff1 = ( ( checkpoint > first ) ? checkpoint - first : first - checkpoint );
  uint64_t diff2 = ( ( checkpoint > second ) ? checkpoint - second : second - checkpoint );
  return ( ( diff1 < diff2 ) ? first : second );
}
