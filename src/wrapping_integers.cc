#include "wrapping_integers.hh"
#include "debug.hh"
#include <cstdint>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return zero_point + static_cast<uint32_t>( n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // your code here.
  Wrap32 ckpt_32 { wrap( checkpoint, zero_point ) };
  int32_t signed_diff { static_cast<int32_t>( this->raw_value_ - ckpt_32.raw_value_ ) };
  int64_t result { static_cast<int64_t>( signed_diff ) + static_cast<int64_t>( checkpoint ) };
  return result < 0 ? result + ( 1ULL << 32 ) : result;
}
