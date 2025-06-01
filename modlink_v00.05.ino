#include <SD.h>
#include <SPI.h>

void sdUp() {
  Serial.println("[SD] UP...");
  digitalWrite(4, HIGH);
  delay(50);
}
void sdDown() {
  Serial.println("[SD] DOWN...");
  digitalWrite(4, LOW);
  delay(50);
}
void sdInit() {
  Serial.println("[SD] Init...");
  pinMode(4, OUTPUT);
  sdUp();
}
bool sdCheckFile() {
  bool fileExists = SD.exists("firmware.hex");
  if (fileExists){
    Serial.println("[SD] File found...");
  } else {
    Serial.println("[SD] File NOT found...");
  }
  sdDown();
  return fileExists;
}
void megaUp() {
  Serial.println("[MEGA] UP...");
  digitalWrite(10, HIGH);
  delay(50);
}
void megaDown() {
  Serial.println("[MEGA] DOWN...");
  digitalWrite(10, LOW);
  delay(50);
}
void megaInit() {
  Serial.println("[MEGA] Init...");
  pinMode(10, OUTPUT);
  megaUp();
}
void megaChipErase() {
  Serial.println("[MEGA] Erase...");
  megaSendCommand(0x80, 0x00);
  delay(15);
}
bool megaProgramMode() {
  Serial.println("[MEGA] Program...");
  byte response = megaSendCommand(0x53, 0x00);
  if (response == 0x53) {
    Serial.println("[MEGA] Recieved 0x53 byte[2]\n");
    return true;
  } else {
    Serial.println("[MEGA] Failed...");
    megaUp();
    megaDown();
    return false;
  }
}
const byte megaSendCommand(byte command, byte value) {
  static byte megaResponse;
  Serial.println("[MEGA] Sending Command...");
  megaDown();
  SPI.beginTransaction(SPISettings(5000, MSBFIRST, SPI_MODE0));
  SPI.transfer(0xAC);
  SPI.transfer(command);
  megaResponse = SPI.transfer(0x00);
  SPI.transfer(value);
  SPI.endTransaction();
  return megaResponse;
}
void megaProgramFuses() {
  Serial.println("[MEGA] Writing fuses...");
  megaSendCommand(0xA0, 0xFF);
  megaSendCommand(0xA8, 0xDA);
  megaSendCommand(0xA4, 0xFD);
  megaSendCommand(0xE0, 0x3F);
}
void flashBytes(bool high, uint16_t addr, uint8_t data) {
  SPI.beginTransaction(SPISettings(5000, MSBFIRST, SPI_MODE0));
  SPI.transfer(high ? 0x48 : 0x40); SPI.transfer((addr >> 8) & 0xFF); SPI.transfer(addr & 0xFF); SPI.transfer(data);
  SPI.endTransaction();
  Serial.print("[FLASH] Writing "); Serial.print(high ? "HIGH" : "LOW");
  Serial.print(" byte at 0x"); Serial.print(addr, HEX); 
  Serial.print(" = 0x"); Serial.println(data, HEX);
}
void commitPage(uint16_t addr) {
  SPI.beginTransaction(SPISettings(5000, MSBFIRST, SPI_MODE0));
  SPI.transfer(0x4C); SPI.transfer((addr >> 8) & 0xFF); SPI.transfer(addr & 0xFF); SPI.transfer(0x00);
  SPI.endTransaction();
  delay(15);
}
bool flashHex() {
  Serial.println("[FLASH] Starting...");
  megaDown();
  sdUp();
  File hexFile = SD.open("firmware.hex");
  if (!hexFile) {
    Serial.println("[FLASH] Failed...");
    return;
  }
  uint16_t pageAddr = 0;
  const uint16_t pageSize = 64;
  while (hexFile.available()) {
    String line = hexFile.readStringUntil('\n');
    line.trim();
    if (line.charAt(0) != ':') {
      Serial.println("[FLASH] Error !:");
      continue;
    }
    int byteCount = strtol(line.substring(1, 3).c_str(), NULL, 16);
    int recordType = strtol(line.substring(7, 9).c_str(), NULL, 16);
    if (recordType == 0x01) {
      Serial.println("[FLASH] EOF...");
      break;
    }
    for (int i = 0; i < byteCount; i++) {
      int dataByte = strtol(line.substring(9 + i * 2, 11 + i * 2).c_str(),NULL, 16);
      sdDown();
      megaUp();
      flashBytes(false, pageAddr, dataByte);
      flashBytes(true, pageAddr, 0xFF);
      commitPage(pageAddr);
      megaDown();
      sdUp();
      pageAddr++;
    }
  }
  hexFile.close();
  sdDown();
}
void setup() {
  Serial.begin(115200);
  SPI.begin();
  while (!Serial);
  megaInit();
  sdInit();
  if (sdCheckFile()) {
    megaProgramMode();
    megaChipErase();
    megaProgramFuses();
    flashHex();
  } else {
    Serial.println("[FLASH] Firmware not found...\n");
  }
}
void loop() {}
