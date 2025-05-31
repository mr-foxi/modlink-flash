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
static byte FUSE_L = 0xFF; // Example: Low fuse for external crystal >8MHz, default startup <- Gemini Def, double check!! Value from PlatformIO value
static byte FUSE_H = 0xDA; // Example: High fuse (BOOTSZ=2048W_3800, EESAVE, SPIEN enabled) <- Gemini Def, double check!! Value from PlatformIO value
static byte FUSE_E = 0xFD; // Example: Extended fuse (BODLEVEL=2.7V) <- Gemini Def, double check!! Value from PlatformIO value
// static byte LOCK_BITS_VAL = 0xCF; // Recommended: No further programming/verification (SPM/LPM in App section disabled). 0x3F for no locks.
static byte LOCK_BITS_VAL = 0x3F; // No locks applied
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
  delay(50);
}
void sdDown() {
  Serial.println("[SD] De-Activating microSD...");
  digitalWrite(SD_CS, LOW)
  Serial.println("[SD] CS pin set: LOW");
  delay(50);
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
  delay(50);
}
void megaDown() {
  Serial.println("[MEGA] De-Activating ATmega328P...");
  digitalWrite(MEGA_RESET, LOW);
  Serial.println("[MEGA] Reset pin set: LOW");
  delay(50);
}
void megaInit() {
  Serial.println("\n[MEGA] Initializing ATmega328P...");
  pinMode(MEGA_RESET, OUTPUT);
  Serial.println("[MEGA] Reset pin set mode: OUTPUT");
  sdUp();
}
const byte* megaReadSig() {
  Serial.println("\n[MEGA] Reading chip signature...");
  megaDown();
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
  Serial.print("[MEGA] Chip signature: ");
  for (int i = 0; i < 3; i++) {
    Serial.print("0x");
    if (chipSig[i] < 0x10) Serial.print("0"); // Add prefix 0 if required for visual formatting
    Serial.print(chipSig[i], HEX);
    if (i < 2) Serial.print(" ");
  }
  return chipSig;
}
bool megaCheckSig() {
  const byte* chipSig = megaReadSig();
  if (chipSig[0] == 0x1E && chipSig[1] == 0x95 && chipSig[2] == 0x0F) {
    Serial.println("[MEGA] Chip signature matches " + chip_name_atmega328p + "\n");
    return true;
  } else {
    Serial.println("[MEGA] Chip signature does NOT match the list of known chip signatures \n");
    return false;
  }
}
const byte* megaSendCommand(const byte* megaCommand) {
  static byte megaResponse[4];
  Serial.println("[MEGA] Sending Command...");
  megaDown();
  Serial.print("[MEGA] Chip Response: ");
  SPI.beginTransaction(SPISettings(spiBaud, MSBFIRST, SPI_MODE0));
  for (int i = 0; i < 4; i++) {
    megaResponse[i] = SPI.transfer(megaCommand[i]);
    Serial.print("0x");
    if (megaResponse[i] < 0x10) Serial.print("0");
    Serial.print(megaResponse[i], HEX);
    if (i < 3) Serial.print(" "); 
  }
  SPI.endTransaction();
  Serial.println();
  return megaResponse;
}
void megaChipErase() {
  Serial.println("\n[MEGA] Attempting to erase chip...");
  const byte eraseCommand[4] = {0xAC, 0x80, 0x00, 0x00}; // ATmega328P Datasheet 27.8.3 Serial Programming Instruction set (page:256) 
  const byte* megaResponse = megaSendCommand(eraseCommand);
  delay(15); // Chip erase can take time ~9.3ms max (datasheet: ATmega328P) << GEMINI INFO
  Serial.println("\n[MEGA] Erase chip command sent...");
}
bool megaProgramMode() {
  Serial.println("\n[MEGA] Attempting to enter programming mode...");
  const byte programCommand[4] = {0xAC, 0x53, 0x00, 0x00}; // ATmega328P Datasheet 27.8.3 Serial Programming Instruction set (page:256) 
  const byte* megaResponse = megaSendCommand(programCommand);
  if (megaRespone[2] == 0x53) {
    Serial.println("[MEGA] Recieved 0x53 at expected chip response byte[2]\n");
    return true;
  } else {
    Serial.println("[MEGA] Failed to initiate programming mode...\n");
    return false;
  }
}
void megaProgramFuses() {
  Serial.println("\n[MEGA] Attempting to program chip settings for fuses and lock bits...")
  const byte fuseL[4] = {0xAC, 0xA0, 0x00, FUSE_L}; // ATmega328P Datasheet 27.8.3 Serial Programming Instruction set (page:256) 
  const byte fuseH[4] = {0xAC, 0xA8, 0x00, FUSE_H};
  const byte fuseE[4] = {0xAC, 0xA4, 0x00, FUSE_E};
  const byte lockBits[4] = {0xAC, 0xE0, 0x00, LOCK_BITS_VAL};
  Serial.println("[MEGA] Attempting to program Fuse L...")
  megaSendCommand(fuseL);
  Serial.println("[MEGA] Attempting to program Fuse H...")
  megaSendCommand(fuseH);
  Serial.println("[MEGA] Attempting to program Fuse E...")
  megaSendCommand(fuseE);
  Serial.println("[MEGA] Attempting to program Lock Bits...")
  megaSendCommand(lockBits);
  Serial.println("[MEGA] All commands for fuses and lock bits have been sent...\n")
}
// === Flashing and Parsing Functions ===
// ATmega328P Datasheet 27.8.3 Serial Programming Instruction set (page:256)
// "Write program memory page | $4C | adr MSB | adr LSB | $00"

/* GEM ROUTE
uint8_t hexCharNibble() {
  -
}
uint8_t parseHexByte() {
  -
}
void flushPageBuffer() {
  -
}
*/
// CO 
void flashByte() {
  - 
}
void commitPage() {
  -
}
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

