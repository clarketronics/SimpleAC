#include "Helpers.h"

// Reader class constructor, sets up readers for use.
void Helpers::setupReader(FlashBeep &feedback, NFCReader &nfcReader){
  #ifdef using_RC522
    SPI.begin();  // Initiate  SPI bus.
    delay(2000); // Wait for the SPI to come up.
    nfcReader.PCD_Init(); // Initiate MFRC522
    nfcReader.PCD_SetAntennaGain(nfcReader.RxGain_max); // Set antenna gain to max (longest range).
    byte v = nfcReader.PCD_ReadRegister(nfcReader.VersionReg); // Get the software version.

    if ((v == 0x00) || (v == 0xFF)) {
      #ifdef debug
        Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
        Serial.println(F("SYSTEM HALTED: Check connections."));
      #endif

      // Flash the red LED and beep 3 times.
      feedback.output(SHORT_PERIOD, 3, RGBred);

      // Halt.
      while (true);
    }

    // Reader is ready.
    #ifdef debug
      Serial.println(F("Reader is ready."));
    #endif
  #endif

  #ifdef using_PN532
    nfcReader.begin();
    uint32_t versiondata = nfcReader.getFirmwareVersion();

    // Check the device is connected, error if not.
    if (!versiondata) {
        // Error state outputs (debug messages).
        #ifdef debug
            Serial.print("Didn't find PN53x board");
        #endif

        // Flash the red LED and beep 3 times.
        feedback.output(SHORT_PERIOD, 3, RGBred);

        // Halt.
        while(true);
    }

    // Got ok data, print it out!
    #ifdef debug
        Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
        Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
        Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
    #endif

    /* Set the max number of retry attempts to read from a card
    This prevents us from waiting forever for a card, which is
    the default behaviour of the PN532. */
    nfcReader.setPassiveActivationRetries(0xFF);

    // configure board to read RFID tags
    nfcReader.SAMConfig();

    // Reader is ready.
    #ifdef debug
        Serial.println(F("Reader is ready."));
    #endif      
  #endif
}

// Read a card and return a bool.
bool Helpers::readCard(Data &data, NFCReader &nfcReader){
  #ifdef using_PN532
    bool success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
    // On each loop check if a ISO14443A card has been found.
    success = nfcReader.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

    if (success) {
      for (int i = 0; i < uidLength; i++) {
        memcpy(data.readCard + i, uid + i, 1);
      }

      return true;
    } else {
      return false;
    }
  #endif

  #ifdef using_RC522
    // On each loop check if a ISO14443A card has been found.
    if ( ! nfcReader.PICC_IsNewCardPresent()) {
      return false;
    }

    // Select one of the cards
    if ( ! nfcReader.PICC_ReadCardSerial()) {
      return false;
    }
    
    for (byte i = 0; i < nfcReader.uid.size; i++) {
      data.readCard[i] = nfcReader.uid.uidByte[i];
    }
    return true;
  #endif
}

// If the uid is 4bytes followed by 3bytes of 00 remove them.
bool Helpers::check4Byte(Data &data) {
  int byteCount = 0;
  for (int i = 0; i < 7; i++) {
    if (data.readCard[i] != 00) {
      byteCount++;
    }
  }

  if (byteCount > 4){
    return false;
  } else {
    for (int i = 0; i < 4; i++) {
      memcpy(data.smallCard + i, data.readCard + i, 1);
    }
    return true;
  }
}

// Read a UID byte from the eeprom.
byte Helpers::readUIDbit(int startPosition, int index) {       
  return EEPROM.read(startPosition + index);
}

// Check read card against master uid
bool Helpers::checkmaster(byte Card[], Data &data){
    if (memcmp(data.masterCard, Card, data.masterSize) == 0){
      return true;
    }
  return false;
}

// Print hex messages with leading 0's
void Helpers::printHex8(uint8_t *data, uint8_t length) { 
  for (int i=0; i<length; i++) { 
    if (data[i]<0x10) {Serial.print("0");} 
    Serial.print(data[i],HEX); 
    Serial.print(" "); 
  }
}

// Find the card to remove from list
card Helpers::findCardtoRemove(byte Card[], Data &data){
  // Define a card that will be returned if scanned card is not in list.
  card errorCard;
  for (int i = 0; i < 7; i++) {
    memcpy(errorCard.uid + i, data.errorUID + i, 1);
  }

  // Try to find the card in the list.
  for (int i = 0; i < data.authorisedCards.size(); i++){
    card curentCard = data.authorisedCards.get(i);

    if (memcmp(curentCard.uid, Card, curentCard.size) == 0){
      return curentCard;
    }
  }
  return errorCard;
}

// Check read card against authorised uid's
bool Helpers::checkCard(int size, Data &data){

  if (size == fourByte) {
    for (int i = 0; i < data.authorisedCards.size(); i++){
      card curentCard = data.authorisedCards.get(i);
      if (memcmp(curentCard.uid, data.smallCard, 4) == 0){
        return true;
      }
    }
    return false;
  } else if (size == sevenByte) {
    for (int i = 0; i < data.authorisedCards.size(); i++){
      card curentCard = data.authorisedCards.get(i);
      if (memcmp(curentCard.uid, data.readCard, 7) == 0){
        return true;
      }
    }
    return false;
  } else {
    return false;
  }
}

//clear card read arrays and reset readstate booleans:
void Helpers::cleanup(Data &data){
    // Clear readCard arrays.
    for (int i = 0; i < 7; i++) {
      data.readCard[i] = 00;
    }

    for (int i = 0; i < 4; i++) {
      data.smallCard[i] = 00;
    }

    data.itsA4byte = false;
}

// Clear button held during boot
bool Helpers::monitorClearButton(uint32_t interval) {
    uint32_t now = (uint32_t)millis();
    while ((uint32_t)millis() - now < interval)  {
      // check on every half a second
      if (((uint32_t)millis() % 500) == 0) {
        if (digitalRead(clearButton) != HIGH)
          return false;
      }
    }
  return true;
}

// If programming button pulled low during boot then do this.
void Helpers::cleanSlate(Data &data, NFCReader &nfcReader, FlashBeep &feedback) {

  #if defined(using_Buzzer) && defined(using_LED)
    // Flash blue led and beep a 3 times.
    feedback.output(SHORT_PERIOD, 3, RGBblue);
  #endif

  #if defined(using_Buzzer) && !defined(using_LED)
      // Beep 4 times
      feedback.output(SHORT_PERIOD, 4);
  #endif

  // Print a serial message (if enabled).
  #ifdef debug
    Serial.println(F("-------------------"));
    Serial.println(F("Clear mode!"));
    Serial.println(F("Please release the clear button / ground connection. Once released we can continue."));
  #endif

  // Wait until the programming button / pin goes high.
  while (digitalRead(clearButton) == LOW) {}

  // Print serial message (if enabled).
  #ifdef debug
    Serial.println(F("-------------------"));
    Serial.println(F("Scan master card to clear all cards"));
    Serial.println(F("Reset to resume normal opperation"));
  #endif

  // Wait until we scan a card befor continuing.
  while (!readCard(data, nfcReader)){}
  

  // Read Master Card's UID from EEPROM
  for ( int i = 0; i < data.masterSize; i++ ) {          
    data.masterCard[i] = EEPROM.read(masterUIDSizeLocation + 1 + i);
  }

  #ifdef debug
    printHex8(data.masterCard, data.masterSize); // Print each character of the mastercard UID.
    Serial.println("");
  #endif

  // See if scanned card is the master.
  if (checkmaster(data.readCard, data)) {
    #ifdef debug
      Serial.println(F("-------------------"));
      Serial.println(F("This is your last chance"));
      Serial.println(F("You have 10 seconds to Cancel"));
      Serial.println(F("This will be remove all records and cannot be undone"));
    #endif

    #if defined(using_Buzzer) && defined(using_LED)
      // Flash red, green, red led and beep a 3 times.
      feedback.output(SHORT_PERIOD, 1, RGBred);
      feedback.output(SHORT_PERIOD, 1, RGBgreen);
      feedback.output(SHORT_PERIOD, 1, RGBred);
    #endif

    #if defined(using_Buzzer) && !defined(using_LED)
      // Beep 4 times
      feedback.output(SHORT_PERIOD, 4);
    #endif

    bool buttonState = monitorClearButton(10000); // Wait for the button to be pressed for 10 seconds.

    // If button still be pressed, wipe EEPROM
    if (buttonState && digitalRead(clearButton) == HIGH) {      
      // Loop through eeprom and remove everything.
      for (unsigned int i = 0 ; i < EEPROM.length() ; i++) {
        EEPROM.update(i, 0);
      }

      #ifdef debug
        Serial.println(F("-------------------"));
        Serial.println(F("All data erased from device"));
        Serial.println(F("Please reset to re-program Master Card"));
      #endif

      // turn the green LED on when we're done
      digitalWrite(RGBgreen, HIGH);
      digitalWrite(Buzzer, HIGH);

      // Halt.
      while (1);
    }
    #ifdef debug
      Serial.println(F("-------------------"));
      Serial.println(F("Master Card Erase Cancelled"));
      Serial.println(F("Reset to resume normal opperation"));
    #endif


    // Halt, user needs to power cycle.
    while (1);
  }    
}

// Read all values from eeprom at start of day.
void Helpers::onBoot(Data &data, NFCReader &nfcReader, FlashBeep &feedback){
  // Check to see if we are entering clearing mode.
  if (digitalRead(clearButton) == LOW) {
    cleanSlate(data, nfcReader, feedback);
  }

  // Check if master is defined, halt if not.
  if (EEPROM.read(masterDefinedLocation) != 143) {
    #ifdef debug
      Serial.println(F("-------------------"));
      Serial.println(F("No master card defined."));
      Serial.println(F("Please scan a master card.")); 
      Serial.println(F("-------------------"));
    #endif

    #if defined(using_Buzzer) && defined(using_LED)
      // Flash red led twice and green onece, beep a 3 times.
      feedback.output(SHORT_PERIOD, 2, RGBred);
      feedback.output(SHORT_PERIOD, 1, RGBgreen);
    #endif

    #if defined(using_Buzzer) && !defined(using_LED)
        // Beep six times
        feedback.output(SHORT_PERIOD, 6);
    #endif

    // Wait until we scan a card befor continuing.
    while (!readCard(data, nfcReader)){}

    // Check to see if 4 or 7 byte uid.
    data.itsA4byte = check4Byte(data);

    // Write the scanned card to the eeprom as the master card.
    if (!data.itsA4byte) {
      EEPROM.update(masterUIDSizeLocation, 7);
      data.masterSize = 7;
      for (unsigned int i = 0; i < 7; i++ ) {        
        EEPROM.update(masterUIDSizeLocation + 1 + i, data.readCard[i]);  // Write scanned PICC's UID to EEPROM, start from address 3
      }
    } else {
      data.masterSize = 4;
      EEPROM.update(masterUIDSizeLocation, 4);
      for (unsigned int i = 0; i < 4; i++ ) {        
        EEPROM.update(masterUIDSizeLocation + 1 + i, data.smallCard[i]);  // Write scanned PICC's UID to EEPROM, start from address 3
      }
    }

    // Print the mastercard UID.
    #ifdef debug
      if (!data.itsA4byte) {
        printHex8(data.readCard, 7); 
        Serial.println("");
      } else {
        printHex8(data.smallCard, 4); 
        Serial.println("");
      }
    #endif

    // Write to EEPROM we defined Master Card.
    EEPROM.update(masterDefinedLocation, 143);

    // Clear data.readCard arrays.
    cleanup(data);

    // Make sure user has moved card.
    delay(500);

  }

  // Read the number of cards currently stored, not including master.
  data.cardCount = EEPROM.read(cardCountLocation);

  // Read the master card and place in memory.
  data.masterSize = EEPROM.read(masterUIDSizeLocation);

  for (int i = 0; i < data.masterSize; i++) {
    byte recievedByte = readUIDbit(masterUIDSizeLocation + 1, i);
    memcpy(data.masterCard + i, &recievedByte, 1);
  }

  // Read access cards and place in memory.
  if (data.cardCount != 0) {
    // Itterate through the number of cards.
    int startPos = masterUIDSizeLocation + data.masterSize + 1;
    
    for (int i = 0; i < data.cardCount; i++) {
      // Read cards size.
      int size = EEPROM.read(startPos);

      // initialise a array of the cards size.
      byte buffer[size];
      for (int i = 0; i < size; i++) {
        buffer[i] = 00;
      }

      startPos++;
      // Reads each card UID from eeprom byte by byte.
      for (int j = 0; j < size; j++) {
        byte recievedByte = readUIDbit(startPos, j);
        memcpy(buffer + j, &recievedByte, 1);
      }

      // Creates a card and sets UID and its index (place in list).
      card currentCard;
      
      for (int j = 0; j < size; j++) {
        memcpy(currentCard.uid + j, buffer + j, 1);
      }

      currentCard.index = i;
      currentCard.size = size;

      // Adds the card the the linked list (where it is referenced from during run time).
      data.authorisedCards.add(currentCard);

      startPos = startPos + size;

      // Print out card details.
      #ifdef debug
        Serial.println(F("-------------------"));
        Serial.print(F("Card number: "));
        Serial.println(currentCard.index);
        Serial.print(F("Card size in bytes: "));
        Serial.println(currentCard.size);
        Serial.print(F("Card uid: "));
        printHex8(currentCard.uid, currentCard.size);
        Serial.println(F(""));
      #endif
    }
    // Output to show number of access cards defined.
    #ifdef debug
      Serial.println(F("-------------------"));          
      Serial.print(F("There are "));
      Serial.print(data.cardCount);
      Serial.print(F(" access cards defined."));
      Serial.println("");
    #endif        

  } else {
    // Output to show no access cards defined.
    #ifdef debug
      Serial.println(F("-------------------"));
      Serial.println(F("No access cards defined."));
    #endif

    #if defined(using_Buzzer) && defined(using_LED)
      // Flash red, red, blue and buzz with each flash.
      feedback.output(SHORT_PERIOD, 2, RGBred);
      feedback.output(SHORT_PERIOD, 1, RGBblue);
    #endif

    #if defined(using_Buzzer) && !defined(using_LED)
        // Beep five times
        feedback.output(SHORT_PERIOD, 5);
    #endif

    // Enter learning mode because master was just set.
    data.state = cardIsMaster;
  }
}

void Helpers::waitForSerial(unsigned long timeout_millis) {
  unsigned long start = millis();
  while (!Serial) {
    if (millis() - start > timeout_millis)
      break;
  }
}