## Pin connections.
These are the pin connections between the `MH-ET ESP32 D1 minikit` and the board with the ILI9441 TFT/touch screen/cardreader.

| PIN NO | TFT NAME        | ESP32 PIN          |
|:------:|:--------------- | -----------------: | 
| 1      | DC              | 27                 |
| 2      | SCLK            | 25                 |
| 3      | MOSI            | 32                 |
| 4      | MISO            | 14                 |
| 5      | CS TFT          |  4                 |
| 6      | CS SD           |  0                 |
| 7      | RST TFT         |  (RST) -1          |
| 8      | PWM TFT         |  2                 |
| 9      | IRQ_TOUCH       | ??                 |

10k pull-up resistors are needed for high speed operation on MOSI and CS lines.<br>
See https://github.com/espressif/esp-idf/tree/master/examples/storage/sd_card

## More info:<br>
http://www.microchip.com/forums/m148739.aspx
http://www.avrfreaks.net/forum/why-have-pullup-mosi-and-miso
