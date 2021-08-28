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

//------Card size config------
enum cardSize{
    fourByte,
    sevenByte
};

//------EEPROM locations------
#define unauthScanCountLocation 2
#define masterDefinedLocation 0
#define masterUIDSizeLocation 3
#define cardCountLocation 1

//------Clear button------
#define clearButton A0

//------Reader wrapper------
#if defined(using_PN532)
    typedef PN532 NFCReader;
#elif defined(using_RC522)        
    typedef MFRC522 NFCReader;
#endif


class Helpers {
    public:
        bool check4Byte(Data &data);
        byte readUIDbit(int startPosition, int index);
        bool checkmaster(byte Card[], Data &data);
        void printHex8(uint8_t *data, uint8_t length);
        card findCardtoRemove(byte readCard[], Data &data);
        bool checkCard(int size, Data &data);
        void cleanup(Data &data);
        bool readCard(Data &data, NFCReader &nfcReader);
        void setupReader(FlashBeep &feedback, NFCReader &nfcReader);
        void onBoot(Data &data, NFCReader &nfcReader, FlashBeep &feedback);
        void waitForSerial(unsigned long timeout_millis);

    private:
        void cleanSlate(Data &data, NFCReader &nfcReader, FlashBeep &feedback);
        bool monitorClearButton(uint32_t interval);
};

