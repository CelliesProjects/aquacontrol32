static inline __attribute__((always_inline)) void threeDigitPercentage( const float percentage, char *result, const uint8_t buffsize )
{
  if ( percentage < 0.01 || percentage > 99.99 )
  {
    snprintf( result, buffsize, " %3.0f%% ", percentage );
  }
  else if ( percentage < 10 )
  {
    snprintf( result,  buffsize , " %1.2f%% ", percentage );
  }
  else
  {
    snprintf( result,  buffsize , " %2.1f%% ", percentage );
  }
}
