## Pin connections.
These are the pin connections between the `MH-ET ESP32 D1 minikit` and the board with the ILI9441 TFT/touch screen/cardreader.

| PIN NO | TFT NAME        | ESP32 PIN          | REMARK      |
|:------:|:---------------:| :----------------: | :---------: |
| 1      | TFT DC          | 27                 |             |
| 2      | SCLK            | 25                 |             |
| 3      | MOSI            | 32                 | 10K PULLUP  |
| 4      | TFT RST         | 12                 |             |
| 5      | TFT CS          |  4                 | 10K PULLUP  |
| 6      | SD CS           |  0                 | 10K PULLUP  |
| 7      | TFT LED         |  2                 | VIA BC547   |
| 8      | MISO            | 39                 |             |
| 9      | TOUCH CS        | 33                 | 10K PULLUP  |
| 10     | TOUCH IRQ       | 35                 | 10K PULLUP  |

10k pull-up resistors are needed for high speed operation on MOSI and CS lines.<br>
See https://github.com/espressif/esp-idf/tree/master/examples/storage/sd_card

## More info:
http://www.microchip.com/forums/m148739.aspx<br>
http://www.avrfreaks.net/forum/why-have-pullup-mosi-and-miso
