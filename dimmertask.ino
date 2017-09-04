void dimmerTask ( void * pvParameters )
{
  while (1)
  {
    for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
    {
      channel[thisChannel].currentPercentage = mapFloat( brightness, 0, LEDC_PWM_DEPTH, 0, 100 );
      ledcWrite( thisChannel, brightness );
    }

    // change the brightness for next time through the loop:
    brightness = brightness + fadeAmount * 2;

    // reverse the direction of the fading at the ends of the fade:
    if (brightness <= 0 || brightness >= LEDC_PWM_DEPTH )
    {
      fadeAmount = -fadeAmount;
    }
    vTaskDelay( 250 / portTICK_PERIOD_MS );
  }
}

