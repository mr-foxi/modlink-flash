/*
  FIRMWARE FLASHER FOR ATMEGA328P WITH ARDUINO NANO, WITH ADAFRUIT MICROSD BREAKOUT FOR STORAGE
  Adafruit microSD breakout (ID:254)
  Atmel Mega 328P
  Arduino Nano

 ** SDO - Nano.D11
 ** SDI - Nano.D12
 ** CLK - Nano.D13
 ** MICROSD CS - Nano.D4
 ** RESET FOR ATMEGA328P - Nano.D10

  BY MR-FOXI MAY/JUNE 2025
*/
/*  HERE IS A FUNCTION LIST
SD:
  - sdUP
  - sdDown
  - sdInit
  - sdCheckFile (bool)
  - sdReadFileChar
  - sdReadFile
ATmega328P: 
  - megaUp
  - megaDown
  - megaInit
  - megaReadSig
  - megaCheckSig
  - megaProgramMode
  - megaChipErase
  - megaWriteFuse
  - megaProgramFuses
Flashing:
  - flashHex (bool)
  - hexCharNibble (gemini)
  - parseHexByte (gemini)
  - flushPageBuffer (gemini)
  - flashByte (copilot)
  - commitPage (copilot)
/* !#!#!#!#!# START: INCLUDED LIBRARIES #!#!#!#!#! */
#include <SD.h>
#include <SPI.h>
/* !#!#!#!#!# END: INCLUDED LIBRARIES #!#!#!#!#! */

/* !#!#!#!#!# START: VARIABLES #!#!#!#!#! */
// === User Variables ===
const char* targetFile = "firmware.hex";
// ===  Pin Definition Variables ===
const int SD_CS = 4; // Micro-SD CS Pin
const int MEGA_RESET = 10; // ATmega328p Reset Pin
// ===  Serial Variables ===
const long serialBaud = 115200; // USB Serial Communication Baud Rate
// ===  SPI Variables ===
const long spiBaud = 100000; // SPI Communication Baud Rate
// === Chip Signatures ===
const byte chip_signature_atmega328p[3] = {0x1E, 0x95, 0x0F};
const String chip_name_atmega328p = "ATmega328P";
// ===  ATmega328P Fuse and Lock Bytes ===
const byte FUSE_L = 0xFF; // Example: Low fuse for external crystal >8MHz, default startup <- Gemini Def, double check!! Value from PlatformIO value
const byte FUSE_H = 0xDA; // Example: High fuse (BOOTSZ=2048W_3800, EESAVE, SPIEN enabled) <- Gemini Def, double check!! Value from PlatformIO value
const byte FUSE_E = 0xFD; // Example: Extended fuse (BODLEVEL=2.7V) <- Gemini Def, double check!! Value from PlatformIO value
 /* GEMINI SUGGESTED vv
const byte LOCK_BITS_VAL = 0xCF; // Recommended: No further programming/verification (SPM/LPM in App section disabled). 0x3F for no locks.
 */// GEMINI SUGGESTED ^^ Prevent Any Further Programming? Maybe good for security??
const byte LOCK_BITS_VAL = 0x3F; // No locks applied
// === Gemini Parsing Variables ===
uint8_t pageDataBuffer[PAGE_SIZE_BYTES];
int bytesInPageBuffer = 0;
uint16_t currentBufferPageBaseWordAddress = 0; // The base word address of the page currently in pageDataBuffer
const uint16_t PAGE_SIZE_WORDS = 64;       // Page size in words for ATmega328P
const uint16_t PAGE_SIZE_BYTES = PAGE_SIZE_WORDS * 2;
/* !#!#!#!#!# END: VARIABLES #!#!#!#!#! */

/* !#!#!#!#!# START: FUNCTIONS #!#!#!#!#! */
// === SD Functions ===
void sdUp() {
  Serial.println("[SD] Activating microSD...");
  digitalWrite(SD_CS, HIGH)
  Serial.println("[SD] CS pin set: HIGH");
  delay(10);
}
void sdDown() {
  Serial.println("[SD] De-Activating microSD...");
  digitalWrite(SD_CS, LOW)
  Serial.println("[SD] CS pin set: LOW");
  delay(10);
}
void sdInit() {
  Serial.println("\n[SD] Initializing microSD...");
  pinMode(SD_CS, OUTPUT);
  Serial.println("[SD] CS pin set mode: OUTPUT");
  sdUp();
}
bool sdCheckFile(String checkFile) {
  Serial.println("\n[SD] Checking for file: " + checkFile);
  sdUp();
  bool fileExists SD.exists(checkFile);
  sdDown();
  Serial.println("[SD] File check: " + targetFile + (fileExists ? "exists." : "does NOT exist."));
  return fileExists;
}
// void sdReadFileChar(File readFile) {
//   Serial.println("[SD] Dumping raw file " + readFile + " contents to terminal...");
//   sdUp();
//   if (!readFile) {
//     Serial.println("[SD] Failed to open " + readFile );
//     return;
//   }
//   Serial.println("[SD] Here is the raw file character by character...");
//   while (hexFile.available()) {
//     char c = hexFile.read();
//     Serial.print(c);
//   }
// }
void sdReadFile(File readFile) {
  Serial.println("\n[SD] Dumping raw file " + readFile + " lines to terminal...");
  if (!readFile) {
    Serial.println("[SD] Failed to open " + readFile + "\n");
    return;
  }
  // Serial.println("[SD] Here is the file line by line...");
  while (readFile.available()) {
    String line = readFile.readStringUntil('\n').trim();
    Serial.println("[SD] " + readFile + " line: " + line);
    Serial.println("[SD] End of file...\n");
  }
}
// === ATmega328P Functions
void megaUp() {
  Serial.println("[MEGA] Activating ATmega328P...");
  digitalWrite(MEGA_RESET, HIGH);
  Serial.println("[MEGA] Reset pin set: HIGH");
  delay(10);
}
void megaDown() {
  Serial.println("[MEGA] De-Activating ATmega328P...");
  digitalWrite(MEGA_RESET, LOW);
  Serial.println("[MEGA] Reset pin set: LOW");
  delay(10);
}
void megaInit() {
  Serial.println("\n[MEGA] Initializing ATmega328P...");
  pinMode(MEGA_RESET, OUTPUT);
  Serial.println("[MEGA] Reset pin set mode: OUTPUT");
  sdUp();
}
const byte* megaReadSig() {
  megaUp();
  Serial.println("\n[MEGA] Reading chip signature...");
  static byte chipSig[3];
  SPI.beginTransaction(SPISettings(spiBaud, MSBFIRST, SPI_MODE0));
  // Read signature byte 0 (Manufacturer ID)
  SPI.transfer(0x30); SPI.transfer(0x00); SPI.transfer(0x00); chipSig[0] = SPI.transfer(0x00);
  // Read signature byte 1 (Device ID Part 1)
  SPI.transfer(0x30); SPI.transfer(0x00); SPI.transfer(0x01); chipSig[1] = SPI.transfer(0x00);
  // Read signature byte 2 (Device ID part 2)
  SPI.transfer(0x30); SPI.transfer(0x00); SPI.transfer(0x02); chipSig[2] = SPI.transfer(0x00);
  SPI.endTransaction();
  // Serial.println("[MEGA] Chip signature: 0x" + String(sig[0], HEX) + "0x" + String(sig[1], HEX) + "0x" + String(sig[2], HEX) + "\n");
  Serial.print("[MEGA] Chip Signature: ");
  for (int i = 0; i < 3; i++) {
    Serial.print("0x");
    if (chipSig[i] < 0x10) Serial.print("0"); // Add prefix 0 if required for visual formatting
    Serial.print(chipSig[i], HEX);
    if (i < 2) Serial.print(" ");
  }
  megaDown();
  return chipSig;
}
bool megaCheckSig() {
  const byte* chipSig = megaReadSig();
  if (chipSig[0] == 0x1E && chipSig[1] == 0x95 && chipSig[2] == 0x0F) {
    Serial.println("[MEGA] Chip Signature matches " + chip_name_atmega328p + "\n");
    return true;
  } else {
    Serial.println("[MEGA] Chip Signature does NOT match the list of known chip signatures \n");
    return false;
  }
}
void megaProgramMode() {
  -
}
void megaChipErase() {
  -
}
void megaWriteFuse() {
  -
}
void megaProgramFuses() {
  - 
}
// === Flashing and Parsing Functions ===
/* GEMINI ROUTE
uint8_t hexCharNibble() {
  -
}
uint8_t parseHexByte() {
  -
}
void flushPageBuffer() {
  -
}
*//* COPILOT ROUTE
void flashByte() {
  - 
}
void commitPage() {
  -
}
*/
bool flashHex() {
  -
}
void 
/* !#!#!#!#!# END: FUNCTIONS #!#!#!#!#! */

/* !#!#!#!#!# START: ADRDUINO SETUP FUNCTION #!#!#!#!#! */

void setup() {
  // put your setup code here, to run once:

}

/* !#!#!#!#!# END: ADRDUINO SETUP FUNCTION #!#!#!#!#! */
/* !#!#!#!#!# START: ADRDUINO LOOP FUNCTION #!#!#!#!#! */

void loop() {
  // put your main code here, to run repeatedly:

}

/* !#!#!#!#!# END: ADRDUINO LOOP FUNCTION #!#!#!#!#! */

