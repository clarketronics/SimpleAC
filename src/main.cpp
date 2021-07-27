/* NOTES: 
  * If LED's and Buzzer are enabled the authorised and unautorised blinks / buzzes 
    will be which ever value is greater.

  * If LED's and Buzzer are enabled the interval between blinks / buzzes 
    will be which ever value is greater.

  * Pull LT pin (D2) LOW at startup and you will enter a clear mode
    (this is shown by serial messages, 3 blue LED flashes and beeps) to clear the eeprom.
*/

//-------Global variables and includes-------
#include <Arduino.h>
#include <EEPROM.h>
#include "Config.h"
#include "FlashBeep.h"
#include "Helpers.h"
#include "Data.h"
#define unauthScanCountLocation 2
bool enabled = false;

// Catch idiots defining both reader.
#if defined(using_RC522) && defined(using_PN532) 
#error "You can't define both readers at the same time!."
#endif

//-------MP3 player config-------
#ifdef usingMP3
  // These are user defined variables.
  #define volume 30 // Set the volume of the mp3 played 0~30.
  #define startupTrack 1 // Track played when MP3 player boots.
  #define authorisedTrack 2 // Track played when an autorised card / implant is scanned.
  #define unauthorisedTrack 3 // Track played when an autorised card / implant is scanned.
  #define shutdownTrack 4 // Track played when an autorised card / implant is scanned and the device is active.

  // These are needed to function, no touching.
  #include "SoftwareSerial.h"
  #include "DFRobotDFPlayerMini.h"
  SoftwareSerial mySoftwareSerial(8, 7); // Create SoftwareSerial instance (RX pin, TX pin).
  DFRobotDFPlayerMini myDFPlayer;
  void printDetail(uint8_t type, int value);
#endif

//-------Relay config-------
#ifdef usingRelays
  // These are user defined variables.
  int relay1Delay = 1000; // Time to wait after relay one has turned on before continuing in ms.
  int relay2Delay = 4000; // Time to wait after relay two has turned on before continuing in ms.
  bool enableRelay1 = true; // Enable relay one (do you want to use it?).
  bool enableRelay2 = true; // Enable relay two (do you want to use it?).
  bool disableRelay2 = true; // this is to disable relay two after the delay.

  // These are needed to function, no touching.
  #define relay1 5 // Relay 1.
  #define relay2 6 // Relay 2.

  int relay1State = LOW; // State used to set relay 1.
  int relay2State = LOW; // State used to set relay 2.
#endif

//-------State machine config-------
#define startOfDay 0
#define waitingOnCard 1
#define cardReadSuccessfully 2
#define cardIs4Byte 3
#define cardIs7Byte 4
#define cardIs4ByteMaster 31
#define cardIs4ByteAccess 32
#define cardIs7ByteMaster 41
#define cardIs7ByteAccess 42
#define goToSleep 50
#define noMatch 60

//-------Programmable uid config-------            
#include <LinkedList.h>
#define clearButton A0 // The pin to monitor on start up to enter clear mode.

#define masterDefinedLocation 0
#define cardCountLocation 1
#define masterUIDSizeLocation 3

//-------Sleep config-------
#ifdef sleep
  // These are user defined variables.
  long timeToWait = 10000; // The amount of time in millis to wait before sleeping.

  // These are needed to function, no touching.
  long currentMillis, startMillis; // To keep track of time and time since wake.
  #define touchPin 3 // The pin the touch sensor connects to.

  #include <avr/interrupt.h>
  #include <avr/sleep.h>

  // This is the code that runs when the interrupt button is pressed and interrupts are enabled.
  void wakeUpNow() {}

  // Lets go to sleep.
  void sleepNow() {
    sleep_enable(); // Enables the sleep mode.
    attachInterrupt(0,wakeUpNow, HIGH);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Sleep mode is set here.
    
    #ifdef using_RC522
      nfcReader.PCD_SoftPowerDown(); // Set the power down bit to reduce power consumption.
    #endif

    #ifdef using_PN532
      nfcReader.shutDown(PN532_WAKEUP_SPI); // Set the power down bit to reduce power consumption.
    #endif

    sleep_mode(); // This is where the device is actually put to sleep.

    sleep_disable(); // First thing that is done is to disable the sleep mode.

    state = waitingOnCard;

    #ifdef using_RC522
      nfcReader.PCD_SoftPowerUp(); // Turn the reader back on.
    #endif

    detachInterrupt(0); // Disables the interrupt.

    startMillis = millis(); // Set the last start up time.
  }
  
#endif

// Create instance of custom libraries.
FlashBeep feedback;
Helpers helpers;
Reader reader(feedback);
Data data;

// Setup.
void setup() {
  // feedback to used (Buzzer enable, LED enable).
  #if defined(usingBuzzer) && defined(usingLED)
    feedback.begin(true, true);
  #endif

  // feedback to used (Buzzer disabled, LED enable).
  #if !defined(usingBuzzer) && defined(usingLED)
    feedback.begin(false, true);
  #endif

  // feedback to used (Buzzer enabled, LED disabled).
  #if defined(usingBuzzer) && !defined(usingLED)
    feedback.begin(true, false);
  #endif

  // Initiate a serial communication for debug.
  #ifdef debug
    Serial.begin(115200);   // Initiate a serial communication at 115200 baud.
    while (!Serial) {}  // Wait for serial to be open.
  #endif

  #ifdef usingRelays
    pinMode(relay1, OUTPUT);   // Declaring relay 1 as an output.
    digitalWrite(relay1, LOW); // Setting it to OFF.
    pinMode(relay2, OUTPUT);   //Declaring relay 2 as output.
    digitalWrite(relay2, LOW); // Setting it to OFF.
  #endif

  #ifdef usingMP3
    mySoftwareSerial.begin(9600); // Start SoftwareSerial at 9600 baud.

    // Check the device is connected, error if not.
    if (!myDFPlayer.begin(mySoftwareSerial)) { 
      // Error state outputs (debug messages).
      #ifdef debug
        Serial.println(F("Unable to begin:"));
        Serial.println(F("1.Please recheck the connection!"));
        Serial.println(F("2.Please insert the SD card!"));
      #endif


      // Flash the red LED and beep 2 times.
      feedback.output(SHORT_PERIOD, 2, RGBred);

      // Halt.
      while(true){}
    }

    #ifdef debug
      Serial.println(F("mp3Player ready."));
    #endif

    myDFPlayer.volume(volume);  //Set volume value. From 0 to 30
    myDFPlayer.play(startupTrack);  // Play the second mp3 file.
  #endif

  // Setup the clear button for clearing eeprom.
  pinMode(clearButton, INPUT);
  digitalWrite(clearButton, HIGH);

  #ifdef sleep
    pinMode(touchPin, INPUT);
  #endif
}

// If UID is correct do this.
void authorised() { 
  // Reset the un authorised scan counter.
  EEPROM.update(unauthScanCountLocation, 0);

  // Serial message to notify that UID is valid.
  #ifdef debug
    Serial.println(F("Access authorised."));
  #endif

  // Flash green led and beep a 3 times.
  feedback.output(SHORT_PERIOD, 3, RGBgreen);


  // Play the track assigned when a authorised card / implant is scanned.
  #ifdef usingmp3player
    myDFPlayer.play(authorisedTrack);
  #endif

  // Relay one engaged (for example the main power to a motorcycle think dash)
  #ifdef usingRelays
    if (enableRelay1 == true) { // Check if we're using relay one.
      digitalWrite(relay1, HIGH);
      relay1State = HIGH;

      #ifdef debug
        Serial.println(F("Relay one engaged."));
      #endif

      delay(relay1Delay); //delay x seconds ( for example to allow bike time to prime)
    }
  
  // Relay two engaged and dissengaged after some time (for example the starter of a motorcycle)
    if (enableRelay2 == true) {
      digitalWrite(relay2, HIGH);
      relay2State = HIGH;

      #ifdef debug
        Serial.println(F("Relay two engaged."));
      #endif

      delay(relay2Delay); // Wait x seconds (for example for the motorcycle to tick over).

      if (disableRelay2 == true) {
        digitalWrite(relay2, LOW);
        relay2State = LOW;

        #ifdef debug
          Serial.println(F("Deactivate relay two"));
        #endif
      }
    }
  #endif

  // Set enabled flag.
  enabled = true;
  
  #ifdef sleep
    startMillis = millis(); // Reset timeout counter.
  #endif
}

// If UID is incorrect do this.
void unauthorised() { 
  // Write the number of aunothorised scans to eeprom.
  int unAuthScan = EEPROM.read(unauthScanCountLocation);
  unAuthScan++;
  EEPROM.update(unauthScanCountLocation, unAuthScan);

  // Serial message to notify that UID is invalid.
  #ifdef debug
    Serial.println(F("Access not authorised."));
  #endif

  // Flash red led and beep a 3 times.
  feedback.output(SHORT_PERIOD, 3, RGBred);

  // Play the track assigned when a unauthorised card / implant is scanned.
  #ifdef usingmp3player
    myDFPlayer.play(unauthorisedTrack);
  #endif

  // Lockout all actions for set time.
  if (unAuthScan == 3) {
    digitalWrite(RGBred, HIGH);
    delay(data.lockout[0]);
    digitalWrite(RGBred, LOW);
  } else if (unAuthScan == 4) {
    digitalWrite(RGBred, HIGH);
    delay(data.lockout[1]);
    digitalWrite(RGBred, LOW);
  } else if (unAuthScan == 5) {
    digitalWrite(RGBred, HIGH);
    delay(data.lockout[2]);
    digitalWrite(RGBred, LOW);    
  } else if (unAuthScan >= 6) {
    digitalWrite(RGBred, HIGH);
    delay(data.lockout[3]);
    digitalWrite(RGBred, LOW);    
  }

  #ifdef sleep
    startMillis = millis(); // Reset timeout counter.
  #endif

}

// If scan has been authorised previously were going to cut power.
void shutdown() {
   // Reset the un authorised scan counter.
  EEPROM.update(unauthScanCountLocation, 0);

  // Serial message to notify that UID is valid.
  #ifdef debug
    Serial.println(F("Access authorised, shutting down."));
  #endif

  // Flash green led and beep a 3 times.
  feedback.output(SHORT_PERIOD, 3, RGBgreen);

  // Play the track assigned when a authorised card / implant is scanned.
  #ifdef usingmp3player
    myDFPlayer.play(shutdownTrack);
  #endif

  // Turn off any relays that are on.
  #ifdef usingRelays
    if (relay2State == HIGH)
    { 
      relay2State = LOW;
      digitalWrite(relay2, relay2State);
    }

    if (relay1State == HIGH) {
      relay1State = LOW;
      digitalWrite(relay1, relay1State);
    }
    
  #endif

  // Reset enabled flag.
  enabled = false;

  #ifdef sleep
    startMillis = millis(); // Reset timeout counter.
  #endif

}

// Clear meathods when using master cards.
// Clear button held during boot
bool monitorClearButton(uint32_t interval) {
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
void cleanSlate() {

  // Flash blue led and beep a 3 times.
  feedback.output(SHORT_PERIOD, 3, RGBblue);

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
  helpers.waitForCard(data, reader);

  // Read Master Card's UID from EEPROM
  for ( int i = 0; i < data.masterSize; i++ ) {          
    data.masterCard[i] = EEPROM.read(masterUIDSizeLocation + 1 + i);
  }

  #ifdef debug
    helpers.printHex8(data.masterCard, data.masterSize); // Print each character of the mastercard UID.
    Serial.println("");
  #endif

  // See if scanned card is the master.
  if (helpers.checkmaster(data.readCard, data)) {
    #ifdef debug
      Serial.println(F("-------------------"));
      Serial.println(F("This is your last chance"));
      Serial.println(F("You have 10 seconds to Cancel"));
      Serial.println(F("This will be remove all records and cannot be undone"));
    #endif

    // Flash red, green, red led and beep a 3 times.
    feedback.output(SHORT_PERIOD, 1, RGBred);
    feedback.output(SHORT_PERIOD, 1, RGBgreen);
    feedback.output(SHORT_PERIOD, 1, RGBred);


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
void onBoot(){
  // Check to see if we are entering clearing mode.
  if (digitalRead(clearButton) == LOW) {
    cleanSlate();
  }

  // Check if master is defined, halt if not.
  if (EEPROM.read(masterDefinedLocation) != 143) {
    #ifdef debug
      Serial.println(F("-------------------"));
      Serial.println(F("No master card defined."));
      Serial.println(F("Please scan a master card.")); 
      Serial.println(F("-------------------"));
    #endif

    // Flash red led twice and green onece, beep a 3 times.
    feedback.output(SHORT_PERIOD, 2, RGBred);
    feedback.output(SHORT_PERIOD, 1, RGBgreen);

    // Wait until we scan a card befor continuing.
    helpers.waitForCard(data, reader);

    // Check to see if 4 or 7 byte uid.
    data.itsA4byte = helpers.check4Byte(data);

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
        helpers.printHex8(data.readCard, 7); 
        Serial.println("");
      } else {
        helpers.printHex8(data.smallCard, 4); 
        Serial.println("");
      }
    #endif

    // Write to EEPROM we defined Master Card.
    EEPROM.update(masterDefinedLocation, 143);

    // Clear data.readCard arrays.
    helpers.cleanup(data);

    // Make sure user has moved card.
    delay(500);

  }

  // Read the number of cards currently stored, not including master.
  data.cardCount = EEPROM.read(cardCountLocation);

  // Read the master card and place in memory.
  data.masterSize = EEPROM.read(masterUIDSizeLocation);

  for (int i = 0; i < data.masterSize; i++) {
    byte recievedByte = helpers.readUIDbit(masterUIDSizeLocation + 1, i);
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
        byte recievedByte = helpers.readUIDbit(startPos, j);
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
        helpers.printHex8(currentCard.uid, currentCard.size);
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
      Serial.println(F("Scan master card to add access cards"));
    #endif

    // Flash red, red, blue and buzz with each flash.
    feedback.output(SHORT_PERIOD, 2, RGBred);
    feedback.output(SHORT_PERIOD, 1, RGBblue);
  }
}

// Learning mode (add and remove cards).
void learning(){
  bool learning = true;

  while (learning){

    // Wait for a card to be scanned.
    helpers.waitForCard(data, reader);
    
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
        startMillis = millis();
      #endif

      learning = false;
      data.state = waitingOnCard;

      delay(1000);

      return;
    } else {
      // Card was not master, is it defined?
      if (!data.itsA4byte) {
        cardDefined = helpers.checkCard(Byte7, data);
      } else {
        cardDefined = helpers.checkCard(Byte4, data);
      }

      // If the card is not defined add it to memory.
      if (!cardDefined) {
        // Print message to say it will be added to memory.
        #ifdef debug
          if (!helpers.check4Byte(data)){
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

        if (!helpers.check4Byte(data)){ 
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

// Main loop.
void loop() {
  // State machine!
  switch (data.state) {
    case startOfDay:
      onBoot();
      data.state = waitingOnCard;
      #ifdef sleep
        // Reset sleep timer.
        startMillis = millis();
      #endif
    break;
    case waitingOnCard:
      // Try to read a card.
      if (reader.Read(data)) {
        data.state = cardReadSuccessfully;
      } else {
        data.state = waitingOnCard;
      }
    break;
    case cardReadSuccessfully:
      // Check to see if card is 4 byte or not.
      if (helpers.check4Byte(data)) {
        data.state = cardIs4Byte;
      } else {
        data.state = cardIs7Byte;
      }
    break;
    case cardIs4Byte:
      // Check whether the card read is master or even known.
      if (helpers.checkmaster(data.smallCard, data)) {
        data.state = cardIs4ByteMaster;
      } else if (helpers.checkCard(Byte4, data)) {
        data.state = cardIs4ByteAccess;
      } else {
        data.state = noMatch;
      }      
    break;
    case cardIs7Byte:
      // Check whether the card read is master or even known.
      if (helpers.checkmaster(data.readCard, data)) {
        data.state = cardIs7ByteMaster;
      } else if (helpers.checkCard(Byte7, data)) {
        data.state = cardIs7ByteAccess;
      } else {
        data.state = noMatch;
      }
    break;
    case cardIs7ByteMaster:
    case cardIs4ByteMaster:
      // Output to show entered learing mode.
      #ifdef debug
        Serial.println(F("-------------------"));
        Serial.println(F("Master scanned."));
        Serial.println(F("Scan access card to ADD or REMOVE."));
      #endif 

      // Flash green twice then blue, beep each time.
      feedback.output(SHORT_PERIOD, 2, RGBgreen);
      feedback.output(SHORT_PERIOD, 1, RGBblue);

      // Clear read card arrays.
      helpers.cleanup(data);

      // Enter learning mode (this is blocking).
      learning();

      #ifdef sleep
        // Reset sleep timer.
        startMillis = millis();
      #endif

      // Clear data.readCard arrays.
      helpers.cleanup(data);
    break;
    case cardIs4ByteAccess:
    case cardIs7ByteAccess:
      // If a match is found and the device is not already in an enabled state activate.
      if (!enabled) {
        authorised();
        helpers.cleanup(data);
        data.state = waitingOnCard;
      } else {
        // If a match is found and the device is already in an enabled state shutdown.
        shutdown();
        helpers.cleanup(data);
        data.state = waitingOnCard;
      }
    break;
    case noMatch:
      // If a match is not found unauthorised.
      unauthorised();
      helpers.cleanup(data);
      data.state = waitingOnCard;
    break;
    case goToSleep:
      #ifdef sleep
        sleepNow();
      #endif
    break;
  }

  // Should we sleep yet?
  #ifdef sleep
    // Should we sleep yet?
    currentMillis = millis();
    if(currentMillis >= startMillis + timeToWait) {
      data.state = goToSleep;
    }
  #endif
}