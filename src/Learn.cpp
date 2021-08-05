#include "Learn.h"

// Learning mode (add and remove cards).
void learn(Data &data, NFCReader &nfcReader, FlashBeep &feedback, Helpers &helpers){
  bool learning = true;

  while (learning){

    // Wait for a card to be scanned.
    while (!helpers.readCard(data, nfcReader)){}
    
    // Check if scanned card was master. 
    bool masterFound = false;
    bool cardDefined = false;

    // Check whether card is 4Byte.
    data.itsA4byte = helpers.check4Byte(data);
    
    if (!data.itsA4byte) {
      masterFound = helpers.checkmaster(data.readCard, data);
    } else {
      masterFound = helpers.checkmaster(data.smallCard, data);
    }

    // If the card was master the leave lerning mode.
    if (masterFound){
      // Output to show leaving learing mode.
      #ifdef debug
        Serial.println(F("-------------------"));
        Serial.println(F("Master scanned."));
        Serial.println(F("Exiting learning mode."));
      #endif 

      // Flash the red LED and beep 3 times.
      feedback.output(SHORT_PERIOD, 2, RGBgreen);
      feedback.output(SHORT_PERIOD, 1, RGBblue);

      // Clear data.readCard array.
      helpers.cleanup(data);

      #ifdef sleep
        // Reset sleep timer.
        data.startMillis = millis();
      #endif

      learning = false;
      data.state = waitingOnCard;

      delay(1000);

      return;
    } else {
      // Card was not master, is it defined?
      if (!data.itsA4byte) {
        cardDefined = helpers.checkCard(sevenByte, data);
      } else {
        cardDefined = helpers.checkCard(fourByte, data);
      }

      // If the card is not defined add it to memory.
      if (!cardDefined) {
        // Print message to say it will be added to memory.
        #ifdef debug
          if (!data.itsA4byte){
            Serial.println(F("-------------------"));
            Serial.println(F("Scanned card had a UID of."));
            helpers.printHex8(data.readCard, 7);
            Serial.println("");
            Serial.println(F("It will be added to memory."));
          } else {
            Serial.println(F("-------------------"));
            Serial.println(F("Scanned card had a UID of."));
            helpers.printHex8(data.smallCard, 4);
            Serial.println("");
            Serial.println(F("It will be added to memory."));
          }
        #endif

        // Create a start position.
        int startPos = masterUIDSizeLocation + data.masterSize + 1;

        // Find the next place to write the card.
        for (int i = 0; i < data.authorisedCards.size(); i++) {
          card myCard = data.authorisedCards.get(i);
          startPos = startPos + myCard.size + 1;
        }

        if (!data.itsA4byte){ 
          // Create a card instance for the scanned card.
          card currentCard;
          
          for (int j = 0; j < 7; j++) {
            memcpy(currentCard.uid + j, data.readCard + j, 1);
          }

          currentCard.size = 7;
          currentCard.index = data.cardCount;            
          
          // Write uid size in bytes.
          EEPROM.update(startPos, currentCard.size);
          startPos++;

          // Write uid of card to eeprom.
          for (int i = 0; i < 7; i++) {
            EEPROM.update(startPos + i, data.readCard[i]);
          }

          // add the scanned card to the list
          data.authorisedCards.add(currentCard);

        } else {
          // Create a card instance for the scanned card.
          card currentCard;
          
          for (int j = 0; j < 4; j++) {
            memcpy(currentCard.uid + j, data.readCard + j, 1);
          }

          currentCard.size = 4;
          currentCard.index = data.cardCount;
        
          // Write uid size in bytes.
          EEPROM.update(startPos, currentCard.size);
          startPos++;

          // Write uid of card to eeprom.
          for (int i = 0; i < 4; i++) {
            EEPROM.update(startPos + i, data.smallCard[i]);
          }

          // add the scanned card to the list
          data.authorisedCards.add(currentCard);
        }

        // Update the number of known cards.
        data.cardCount++;
        EEPROM.update(cardCountLocation, data.cardCount);

        // Flash blue twice and red once, beep all 3 times.
        feedback.output(SHORT_PERIOD, 2, RGBblue);
        feedback.output(SHORT_PERIOD, 1, RGBgreen);

        // Gives the user a second to pull the card away.
        delay(1000);

      }

      // If the card is defined remove it from memory.
      if (cardDefined) {
        // Print card is defined.
        #ifdef debug
          if (!data.itsA4byte){
            Serial.println(F("-------------------"));
            Serial.println(F("Scanned card had a UID of."));
            helpers.printHex8(data.readCard, 7);
            Serial.println("");
            Serial.println(F("It will be removed from memory."));
          } else {
            Serial.println(F("-------------------"));
            Serial.println(F("Scanned card had a UID of."));
            helpers.printHex8(data.smallCard, 4);
            Serial.println("");
            Serial.println(F("It will be removed from memory."));
          }
        #endif

        // Find where in the list the card to be removed is.
        card currentCard;
        if (!data.itsA4byte) {          
          currentCard = helpers.findCardtoRemove(data.readCard, data);
        } else {
          currentCard = helpers.findCardtoRemove(data.smallCard, data);
        }

        // Check to see if the current card is the errorUID (ERROR!!).
        if (memcmp(currentCard.uid, data.errorUID, 7) == 0) {
          #ifdef debug
            if (!data.itsA4byte){
            Serial.println(F("-------------------"));
            Serial.println(F("Scanned card had a UID of."));
            helpers.printHex8(data.readCard, 7);
            Serial.println("");
            Serial.println(F("It was not found."));
          } else {
            Serial.println(F("-------------------"));
            Serial.println(F("Scanned card had a UID of."));
            helpers.printHex8(data.smallCard, 4);
            Serial.println("");
            Serial.println(F("It was not found."));
          }
          #endif

          // Flash red, blue then red, beep each time.
          feedback.output(SHORT_PERIOD, 1, RGBred);
          feedback.output(SHORT_PERIOD, 1, RGBblue);
          feedback.output(SHORT_PERIOD, 1, RGBred);
          
          return;            
        }

        // Remove the card from the linked list.
        data.authorisedCards.remove(currentCard.index);

        // Go through the eeprom and remove the blank space.
        int startPos = masterUIDSizeLocation + data.masterSize + 1;
        for (int i = 0; i < data.authorisedCards.size(); i++) {
          card myCard = data.authorisedCards.get(i);
          EEPROM.update(startPos, myCard.size);
          startPos++;

          for (int j = 0; j < myCard.size; j++) {
            EEPROM.update(startPos + j, myCard.uid[j]);
          }

          startPos = startPos + myCard.size;

        }

        // Update the number of known cards.
        data.cardCount--;
        EEPROM.update(cardCountLocation, data.cardCount);

        // Flash blue twice red once, beep each time.
        feedback.output(SHORT_PERIOD, 2, RGBblue);
        feedback.output(SHORT_PERIOD, 1, RGBred);

        // Gives the user a second to pull the card away.
        delay(1000);
      }
    }

    // Clear data.
    helpers.cleanup(data);
  
  }
}
