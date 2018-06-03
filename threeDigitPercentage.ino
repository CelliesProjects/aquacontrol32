static inline __attribute__((always_inline)) void threeDigitPercentage( const float &percentage, char *buffer, const uint8_t &bufferSize, const bool &addPercentSign )
{
  if ( percentage < 0.005 )
  {
    snprintf( buffer, bufferSize, addPercentSign ? "  0%%  " : "  0  " );
  }
  else if ( percentage == 100 )
  {
    snprintf( buffer, bufferSize, addPercentSign ? " 100%% " : " 100 " );
  }
  else if ( percentage < 10 )
  {
    snprintf( buffer,  bufferSize , addPercentSign ? " %1.2f%% " : " %1.2f ", percentage );
  }
  else
  {
    snprintf( buffer,  bufferSize , addPercentSign ? " %2.1f%% " : " %2.1f ", percentage );
  }
}
