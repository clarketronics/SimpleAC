#pragma once
#include <Arduino.h>
#include <LinkedList.h>

//------Card config (for linked list)------
struct card {
  public:
    byte uid[7];
    int size;
    int index;
};

//------State machine config------
enum states{    
    startOfDay,
    waitingOnCard,
    cardReadSuccessfully,
    cardIs4Byte,
    cardIs7Byte,
    cardIs4ByteMaster,
    cardIs4ByteAccess,
    cardIs7ByteMaster,
    cardIs7ByteAccess,
    goToSleep,
    noMatch
};

class Data {
    public:
        byte errorUID[7] = {0x45, 0x52, 0x52, 0x4f, 0x52, 0x21, 0x21};
        long lockout[4] = {60000, 120000, 300000, 900000};
        byte masterCard[7] = {00,00,00,00,00,00,00};
        byte readCard[7] = {00,00,00,00,00,00,00};
        byte smallCard[4] = {00,00,00,00};
        int cardCount, masterSize, state; // Total number of stored cards, size of the master card, state the state machine is in.
        bool itsA4byte; // Is the scanned card a 4 byte.
        unsigned long currentMillis, startMillis; // To keep track of time and time since wake.

        LinkedList<card> authorisedCards = LinkedList<card>();

        // State toggle
        #ifdef accessory
          bool enabled;
        #endif
        
};