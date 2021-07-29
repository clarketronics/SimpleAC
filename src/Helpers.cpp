#include "Helpers.h"

// Reader class constructor, sets up readers for use.
Reader::Reader(FlashBeep &feedback, NFCReader &nfcReader){
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

// Read a card ad return a bool.
bool Reader::Read(Data &data, NFCReader &nfcReader){
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

  if (size == Byte4) {
    for (int i = 0; i < data.authorisedCards.size(); i++){
      card curentCard = data.authorisedCards.get(i);
      if (memcmp(curentCard.uid, data.smallCard, 4) == 0){
        return true;
      }
    }
    return false;
  } else if (size == Byte7) {
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

// Blocks until a card is present at the reader.
void Helpers::waitForCard(Data &data, Reader &reader, NFCReader &nfcReader){ 
    bool cardRead = false;
    while (!cardRead) {
      reader.Read(data, nfcReader);
    }
}