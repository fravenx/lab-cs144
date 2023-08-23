#include "wrapping_integers.hh"
#include <math.h>
using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + uint32_t(n) ;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint32_t offset = raw_value_ - wrap(checkpoint,zero_point).raw_value_;
  uint64_t res = checkpoint + offset;
  if(offset >= (1ul << 31) && res >= 1ul << 32) res -= (1ul << 32);
  return res;
}
