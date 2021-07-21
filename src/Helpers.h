#pragma once
#include "Arduino.h"
#include "EEPROM.h"

unsigned int authorisedOutputs, unauthorisedOutputs, authorisedStates, unauthorisedStates, interval, state;
long lockout[] = {60000, 120000, 300000, 900000};
byte readCard[7] = {00,00,00,00,00,00,00};
byte smallCard[4] = {00,00,00,00};
int cardCount, masterSize;
bool clearEeprom, eepromRead;
bool itsA4byte = false;
byte masterCard[7] = {00,00,00,00,00,00,00};
byte errorUID[7] = {0x45, 0x52, 0x52, 0x4f, 0x52, 0x21, 0x21};

class Helpers {
    public:
        bool check4Byte(byte readCard[7], byte smallCard[4]);
        byte readUIDbit(int startPosition, int index);
};

