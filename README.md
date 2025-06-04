!! WARNING !!
This project is pre-set to apply Fuse and Lock Bit settings. 
If you attempt to use this, ensure these will not create any risks for you.
Check the manufacturers datasheet: https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf

ATmega328P: https://www.microchip.com/en-us/product/atmega328p
Arduino Nano: https://store.arduino.cc/products/arduino-nano
Adafruit MicroSD SPI Break out: https://www.adafruit.com/product/254

Dependency Libraries: https://github.com/adafruit/SdFat

Arduino Nano Pinout:
  D4: MicroSD Breakout Pin CS
  D10: ATmega328p Pin Reset
  D11: SPI - MOSI
  D12: SPI - MISO
  D13: SPI - SCK
  
Arduino Nano USB Baud Rate: 115200

Firmware file must be a pre-compiled hex filed in the microSD root named exactly: "firmware.hex" 
