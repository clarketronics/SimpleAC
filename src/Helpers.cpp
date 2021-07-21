#include "Helpers.h"
#include "EEPROM.h"

// If the uid is 4bytes followed by 3bytes of 00 remove them.
bool Helpers::check4Byte(byte readCard[7], byte smallCard[4]) {
  int byteCount = 0;
  for (int i = 0; i < 7; i++) {
    if (readCard[i] != 00) {
      byteCount++;
    }
  }

  if (byteCount > 4){
    return false;
  } else {
    for (int i = 0; i < 4; i++) {
      memcpy(smallCard + i, readCard + i, 1);
    }
    return true;
  }
}

// Read a UID byte from the eeprom.
byte Helpers::readUIDbit(int startPosition, int index) {       
  return EEPROM.read(startPosition + index);
}

// Check read card against master uid
bool checkmaster(byte readCard[]){
    if (memcmp(masterCard, readCard, masterSize) == 0){
      return true;
    }
  return false;
}