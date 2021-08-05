#pragma once
#include <Arduino.h>
#include <LinkedList.h>

class card {
  public:
    byte uid[7];
    int size;
    int index;
};

class Data {
    public:
        byte errorUID[7] = {0x45, 0x52, 0x52, 0x4f, 0x52, 0x21, 0x21};
        long lockout[4] = {60000, 120000, 300000, 900000};
        byte masterCard[7] = {00,00,00,00,00,00,00};
        byte readCard[7] = {00,00,00,00,00,00,00};
        byte smallCard[4] = {00,00,00,00};
        int cardCount, masterSize, state;
        bool itsA4byte;
        unsigned long currentMillis, startMillis; // To keep track of time and time since wake.

        LinkedList<card> authorisedCards = LinkedList<card>();
        
};