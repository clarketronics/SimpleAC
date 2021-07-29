#pragma once
#include "Arduino.h"
#include "FlashBeep.h"
#include "EEPROM.h"
#include "Data.h"
#include "Config.h"
#include "SPI.h"
#include "PN532_SPI.h"
#include "PN532.h"
#include <MFRC522.h>



#define Byte4 1
#define Byte7 2

#if defined(using_PN532)
    typedef PN532 NFCReader;
#elif defined(using_RC522)        
    typedef MFRC522 NFCReader;
#endif

class Reader {
    public:
        Reader(FlashBeep &feedback, NFCReader &nfcReader);
        bool Read(Data &data, NFCReader &nfcReader);             
};


class Helpers {
    public:
        bool check4Byte(Data &data);
        byte readUIDbit(int startPosition, int index);
        bool checkmaster(byte Card[], Data &data);
        void printHex8(uint8_t *data, uint8_t length);
        card findCardtoRemove(byte readCard[], Data &data);
        bool checkCard(int size, Data &data);
        void cleanup(Data &data);
        void waitForCard(Data &data, Reader &reader, NFCReader &nfcReader);
};

