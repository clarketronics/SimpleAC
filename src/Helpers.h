#pragma once
#include "Arduino.h"
#include "FlashBeep.h"
#include "EEPROM.h"
#include "Data.h"
#include "Config.h"
#include "SPI.h"



#define Byte4 1
#define Byte7 2

#if defined(using_PN532)
    #include "PN532_SPI.h"
    #include "PN532.h"
    PN532_SPI pn532spi(SPI, 10); // Create an SPI instance of the PN532, (Interface, SS pin).
    PN532 nfcReader(pn532spi); // Create the readers class for addressing it directly.
#elif defined(using_RC522)
    #include <MFRC522.h>
    MFRC522 nfcReader(10, 9);   // Create MFRC522 instance, (SS pin, RST pin).
#endif


class Reader {

    public:
        Reader(FlashBeep &feedback);
        bool Read(Data &data);  
             
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
        void waitForCard(Data &data, Reader &reader);
};

