#include "serialport.h"
#include "memory.h"

void EEpromMemory::initMemory() {
  randomSeed(rp2040.hwrand32());
  memMutex->take();
  breakSeal();
  mem.mem.programNumber = PROGRAM_NUMBER;
  mem.mem.programVersionMajor = PROGRAM_VERSION_MAJOR;
  mem.mem.programVersionMinor = PROGRAM_VERSION_MINOR;
  mem.mem.numberOfDevices = NUM_DEVICES;
  mem.mem.isDHCP = true;
  mem.mem.macAddress[0] = 0xDE;
  mem.mem.macAddress[1] = 0xAD;
  mem.mem.macAddress[2] = 0xCC;
  mem.mem.macAddress[3] = random(256);
  mem.mem.macAddress[4] = random(256);
  mem.mem.macAddress[5] = random(256);
  mem.mem.ipAddress[0] = 0;
  mem.mem.ipAddress[1] = 0;
  mem.mem.ipAddress[2] = 0;
  mem.mem.ipAddress[3] = 0;
  mem.mem.dnsAddress[0] = 255;
  mem.mem.dnsAddress[1] = 255;
  mem.mem.dnsAddress[2] = 255;
  mem.mem.dnsAddress[3] = 255;
  mem.mem.subnetMask[0] = 255;
  mem.mem.subnetMask[1] = 255;
  mem.mem.subnetMask[2] = 255;
  mem.mem.subnetMask[3] = 0;
  mem.mem.gatewayAddress[0] = 255;
  mem.mem.gatewayAddress[1] = 255;
  mem.mem.gatewayAddress[2] = 255;
  mem.mem.gatewayAddress[3] = 255;
  String device = "Device";
  char deviceString[NAME_MAX_LENGTH];
  device.toCharArray(deviceString, NAME_MAX_LENGTH);
  for (int i = 0; i < mem.mem.numberOfDevices; i++) {
    memset(mem.mem.deviceName[i], 0, NAME_MAX_LENGTH + 1);
    strncpy(mem.mem.deviceName[i], deviceString, NAME_MAX_LENGTH);
  }
  mem.mem.drift = 0;
  memMutex->give();
  println(PASSED, "Initialize Memory");
}

void EEpromMemory::setDeviceName(byte device, char* name, int length) {
  memMutex->take();
  byte index = device;

  if (device < mem.mem.numberOfDevices) {
    if (length >= 0) {
      int nameLength = length;
      if (length > (NAME_MAX_LENGTH - 1)) nameLength = NAME_MAX_LENGTH - 1;
      memset(mem.mem.deviceName[index], 0, NAME_MAX_LENGTH);
      strncpy(mem.mem.deviceName[index], name, nameLength);
    }
  }
  memMutex->give();
  breakSeal();
}

char* EEpromMemory::getDeviceName(byte device) {
  char* returnVal;
  memMutex->take();
  if (device >= NUM_DEVICES) returnVal = ErrorString;
  returnVal = mem.mem.deviceName[device];
  memMutex->give();
  return returnVal;
}

#ifdef MEMORY

void EEpromMemory::breakSeal() {
  seal = true;
}
void EEpromMemory::setup(Gpio* gpio) {
  connected = false;
  wireMutex->take();
  eeprom->begin();
  connected = eeprom->isConnected();
  wireMutex->give();
  readEEPROM();
  if ((mem.mem.programNumber != PROGRAM_NUMBER) || (mem.mem.programVersionMajor != PROGRAM_VERSION_MAJOR) || (mem.mem.numberOfDevices != NUM_DEVICES)) {
    println(FAILED, "EEPROM memory failed, intializing default values");
    initMemory();
    breakSeal();
  } else {
    println(PASSED, "EEPROM memory success");
  }
  if ((mem.mem.programVersionMinor != PROGRAM_VERSION_MINOR)) {
    mem.mem.programVersionMinor = PROGRAM_VERSION_MINOR;
    breakSeal();
  }
  if (!connected) println(ERROR, "EEPROM Not Connected");
  gpio->setOnline(I2C_EEPROM, connected);
  String memoryString = "Memory Complete: PRG Num: " + String(mem.mem.programNumber) + " PRG Ver: " + String(mem.mem.programVersionMajor) + "." + String(mem.mem.programVersionMinor) + " Num Dev: " + String(mem.mem.numberOfDevices);
  println((connected) ? PASSED : FAILED, memoryString);
}

void EEpromMemory::loop() {
  if (run() && seal) {
    seal = false;
    writeEEPROM();
  }
}

byte EEpromMemory::readEEPROMbyte(unsigned int address) {
  byte result = 0xFF;
  result = eeprom->readByte(address);
  return result;
}

void EEpromMemory::writeEEPROMbyte(unsigned int address, byte value) {
  if (!eeprom->updateByteVerify(address, value) && connected) println(ERROR, "Error in Writing EEPROM");
}

void EEpromMemory::readEEPROM() {
  if (connected) {
    wireMutex->take();
    memMutex->take();
    //println("Reading EEPROM");
    for (unsigned int index = 0; index < sizeof(MemoryStruct); index++) {
      //if (index % 16 == 0) {
      //  print(String(index, HEX));
      //  print(": ");
      //}
      mem.memoryArray[index] = readEEPROMbyte(index);
      //print(String(mem.memoryArray[index], HEX));
      //print("  ");
      //if (index % 16 == 15) println();
    }
    //println();
    memMutex->give();
    wireMutex->give();
  }
}

void EEpromMemory::writeEEPROM() {
  if (connected) {
    wireMutex->take();
    memMutex->take();
    //println("Writing EEPROM");
    for (unsigned int index = 0; index < sizeof(MemoryStruct); index++) {
      //if (index % 16 == 0) {
      //  print(String(index, HEX));
      //  print(": ");
      //}
      writeEEPROMbyte(index, mem.memoryArray[index]);
      //print(String(mem.memoryArray[index], HEX));
      //print("  ");
      //if (index % 16 == 15) println();
    }
    //println();
    memMutex->give();
    wireMutex->give();
  }
}


#else
#pragma GCC warning "No EEPROM Module Included"
void EEpromMemory::breakSeal(){};

void EEpromMemory::setup(Gpio* gpio) {
  initMemory();
}
void EEpromMemory::loop(){};
#endif
