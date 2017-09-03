void dimmerTask ( void * pvParameters )
{
  while (1)
  {
    // set the brightness on LEDC channel 0
    ledcWrite(LEDC_CHANNEL_0, brightness);
    for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
    {
      ledcWrite( thisChannel, brightness );
    }

    // change the brightness for next time through the loop:
    brightness = brightness + fadeAmount;

    // reverse the direction of the fading at the ends of the fade:
    if (brightness <= 0 || brightness >= LEDC_PWM_DEPTH )
    {
      fadeAmount = -fadeAmount;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

