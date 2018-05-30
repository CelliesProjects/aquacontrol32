#ifndef mapFloat_h
#define mapFloat_h

static inline __attribute__((always_inline)) float mapFloat( const float &x, const float &in_min, const float &in_max, const float &out_min, const float &out_max)
{
  return ( x - in_min ) * ( out_max - out_min ) / ( in_max - in_min ) + out_min;
}

#endif

