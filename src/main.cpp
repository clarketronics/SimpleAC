/* NOTES: 
  * If LED's and Buzzer are enabled the authorised and unautorised blinks / buzzes 
    will be which ever value is greater.

  * If LED's and Buzzer are enabled the interval between blinks / buzzes 
    will be which ever value is greater.

  * Pull LT pin (D2) LOW at startup without defining hardcodedUID and you will enter a clear mode
    (this is shown by serial messages, 3 blue LED flashes and beeps) to clear the eeprom.
*/

//-------Definitions- Delete the "//" of the modules you are using----------
//#define debug // Unmark this to enable output printing to the serial monitor.
//#define usingMP3 // Unmark this if using the df mp3player.
//#define usingRC522 // Unmark this if you are using the RC522 13.56MHz NFC-HF RFID reader.
#define usingPN532 // Unmark this if you are using the RC522 13.56MHz NFC-HF RFID reader.
//#define hardcodeUID // Unmark this if you want to hard code your UID's.
#define usingLED // Unmark this if you want to enable the RGB LED.
#define usingBuzzer // Unmark this if you want to enable the Buzzer.
#define usingRelays // Unmark this if you want to enable the Relays.
#define sleep // Unmark this if you want to enable sleep mode (conserves battery).

//-------Global variables and includes-------
#include <Arduino.h>
#include <EEPROM.h>
#define unauthScanCountLocation 2
bool matchfound, enabled, cardRead;
unsigned int authorisedOutputs, unauthorisedOutputs, authorisedStates, unauthorisedStates, interval;
long lockout[] = {60000, 120000, 300000, 900000};
byte readCard[7] = {00,00,00,00,00,00,00};
byte smallCard[4] = {00,00,00,00};

// Catch idiots defining both reader.
#if defined(usingRC522) && defined(usingPN532) 
#error "You can't define both readers at the same time!."
#endif

//-------Hardcoded UID's-------
#ifdef hardcodeUID
  // Change the number in the first [] to the number of UID's wanted put the UID's in brackets as shown.
  // NOTE: The last row dosnt need a comma.
  byte uids[2][7] = {
  {0x04, 0x73, 0x08, 0x3A, 0x94, 0x51, 0x80},
  {0x04, 0x5B, 0x32, 0x22, 0x2F, 0x5C, 0x91},
  };

  // A function to calculate the number of items in an array.
  #define numberofitems(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0])))
#endif

//-------Reader config-------
#ifdef usingRC522
  #include <MFRC522.h>
  #include <SPI.h>
  MFRC522 mfrc522(10, 9);   // Create MFRC522 instance, (SS pin, RST pin).
#endif

#ifdef usingPN532
  #include <SPI.h>
  #include "PN532_SPI.h"
  #include "PN532.h"

  PN532_SPI pn532spi(SPI, 10); // Create an SPI instance of the PN532, (Interface, SS pin).
  PN532 pn532(pn532spi); // Create the readers class for addressing it directly.
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

//-------RGB LED config-------
#ifdef usingLED
  // These are user defined variables.
  const int RGBinterval = 200; // Length of time any LED will flash (milliseconds).
  int authorisedBlinks = 3; // Number of times to blink if correct UID is scanned.
  int unauthorisedBlinks = 3; // Number of times to blink if incorrect UID is scanned.

  // These are needed to function, no touching.
  #define RGBgreen A1 // Rgb green.
  #define RGBblue A2 // Rgb blue.
  #define RGBred A3 // Rgb Red.

  int RGBgreenState = LOW; // State used to set the green LED.
  int RGBblueState = LOW; // State used to set the blue LED.
  int RGBredState = LOW; // State used to set the red LED.

  unsigned long RGBpreviousMillis = 0;        // will store last time LED was updated.
#endif

//-------Buzzer config-------
#ifdef usingBuzzer
  // These are user defined variables.
  const int Buzzerinterval = 200; // Length of time the buzzer will buzz (milliseconds).
  int authorisedBuzzes = 3; // Number of times to buzz if correct UID is scanned.
  int unauthorisedBuzzes = 3; // Number of times to buzz if incorrect UID is scanned.

  // These are needed to function, no touching.
  #define Buzzer 4 // Onboard buzzer.

  int BuzzerState = LOW;

  unsigned long BuzzerpreviousMillis = 0;        // will store last time buzzer was updated.
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

//-------Programmable uid config-------            
#ifndef hardcodeUID
  // These are needed to function, no touching.
  #include <linkedlist.h>
  #define clearButton A0 // The pin to monitor on start up to enter clear mode.
  int cardCount, masterSize;
  bool scorchedEarth, eepromRead;
  bool itsA4byte = false;
  byte masterCard[7] = {00,00,00,00,00,00,00};
  byte errorUID[7] = {0x45, 0x52, 0x52, 0x4f, 0x52, 0x21, 0x21};

  #define masterDefinedLocation 0
  #define cardCountLocation 1
  #define masterUIDSizeLocation 3

  class card {
    public:
      byte uid[7];
      int size;
      int index;
  };

  LinkedList<card> authorisedCards = LinkedList<card>();

  // Read a UID byte from the eeprom.
  byte readUIDbit(int startPosition, int index) {       
    return EEPROM.read(startPosition + index);
  }

  // Find the card to remove from list
  card findCardtoRemove(byte readCard[]){
    // Define a card that will be returned if scanned card is not in list.
    card errorCard;
    for (int i = 0; i < 7; i++) {
      memcpy(errorCard.uid + i, errorUID + i, 1);
    }

    // Try to find the card in the list.
    for (int i = 0; i < authorisedCards.size(); i++){
      card curentCard = authorisedCards.get(i);
      if (memcmp(curentCard.uid, readCard, curentCard.size) == 0){
        return curentCard;
      }
    }
    return errorCard;
  }

  // Check read card against master uid
  bool checkmaster(byte readCard[]){
      if (memcmp(masterCard, readCard, masterSize) == 0){
        return true;
      }
    return false;
  }

#endif

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
    
    #ifdef usingRC522
      mfrc522.PCD_SoftPowerDown(); // Set the power down bit to reduce power consumption.
    #endif

    #ifdef usingPN532
      pn532.shutDown(PN532_WAKEUP_SPI); // Set the power down bit to reduce power consumption.
    #endif

    sleep_mode(); // This is where the device is actually put to sleep.

    sleep_disable(); // First thing that is done is to disable the sleep mode.

    #ifdef usingRC522
      mfrc522.PCD_SoftPowerUp(); // Turn the reader back on.
    #endif

    detachInterrupt(0); // Disables the interrupt.

    startMillis = millis(); // Set the last start up time.
  }
  
#endif

// If the uid is 4bytes followed by 3bytes of 00 remove them.
bool check4Byte() {
  int byteCount = 0;
  for (int i = 0; i < 7; i++) {
    if (readCard[i] != 00) {
      byteCount++;
    }
  }

  if (byteCount > 4){
    return false;
  } else {
    for (int i = 0; i < 4; i++) {
      memcpy(smallCard + i, readCard + i, 1);
    }
    return true;
  }
}

// Flash led / beep x number of times.
void flashBeep(unsigned int states, unsigned int interval, int LEDstate, int  LED) {
  // Flash led / beep set number of times.
    #ifdef usingLED
      bool incrementLED = false; // bool to check if we should increment the state counter.
    #endif

    #ifdef usingBuzzer
      bool incrementBuzz = false; // bool to check if we should increment the state counter.
    #endif

    unsigned long RGBpreviousMillis, BuzzerpreviousMillis;

    for (unsigned int i = 0; i <= states;) {
      unsigned long currentMillis = millis(); // The current uptime.

      #ifdef usingLED
        if ((unsigned long)currentMillis - RGBpreviousMillis >= interval) {
          // save the last time you blinked the LED
          RGBpreviousMillis = currentMillis;

          // if the LED is off turn it on and vice-versa:
          if (LEDstate == LOW) {
            LEDstate = HIGH;
          } else {
            LEDstate = LOW;
          }

          // Set the increment flag.
          incrementLED = true;
        }
        
        // Set the LED.
        digitalWrite(LED, LEDstate);
      #endif

      #ifdef usingBuzzer
        if ((unsigned long)currentMillis - BuzzerpreviousMillis >= interval) {
          // save the last time you buzzed the buzzer.
          BuzzerpreviousMillis = currentMillis;

          // if the buzzer is off turn it on and vice-versa:
          if (BuzzerState == LOW) {
            BuzzerState = HIGH;
          } else {
            BuzzerState = LOW;
          }

          // Set the increment flag.
          incrementBuzz = true;
        }

        // Set the buzzer.
        digitalWrite(Buzzer, BuzzerState);
      #endif

      #if defined(usingLED) && defined(usingBuzzer)
        // Check wheather we need to increment the state counter.
        if (incrementLED == true && incrementBuzz == true) {
          i++; // Increment state counter.
          incrementLED = false; // Reset the increment flag.
          incrementBuzz = false; // Reset the increment flag.
        }
      #endif

      #ifndef usingBuzzer
        if (incrementLED == true) {
            i++; // Increment state counter.
            incrementLED = false; // Reset the increment flag.
        }
      #endif

      #ifndef usingLED
        if (incrementBuzz == true) {
            i++; // Increment state counter.
            incrementBuzz = false; // Reset the increment flag.
        }
      #endif
    }

    // After loop make sure led and buzzer are off.
    #ifdef usingLED
      if (LEDstate == HIGH) {
        LEDstate = LOW;
      }
      digitalWrite(LED, LEDstate);
    #endif

    #ifdef usingBuzzer
      if (BuzzerState == HIGH) {
        BuzzerState = LOW;
      }
      digitalWrite(Buzzer, BuzzerState);
    #endif
}

// Print hex messages with leading 0's
void PrintHex8(uint8_t *data, uint8_t length) { 
  for (int i=0; i<length; i++) { 
    if (data[i]<0x10) {Serial.print("0");} 
    Serial.print(data[i],HEX); 
    Serial.print(" "); 
  }
}

// If UID is correct do this.
void authorised() { 
  // Reset the un authorised scan counter.
  EEPROM.update(unauthScanCountLocation, 0);

  // Serial message to notify that UID is valid.
  #ifdef debug
    Serial.println(F("Access authorised."));
  #endif

  // Flash led / beep a set number of times.
  #ifdef usingLED
  flashBeep(authorisedStates, interval, RGBgreenState, RGBgreen);
  #endif

  // Play the track assigned when a authorised card / implant is scanned.
  #if defined (usingmp3player)
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

  // Flash led / beep a set number of times.
  flashBeep(unauthorisedStates, interval, RGBredState, RGBred);

  // Play the track assigned when a unauthorised card / implant is scanned.
  #if defined (usingmp3player)
    myDFPlayer.play(unauthorisedTrack);
  #endif

  // Lockout all actions for set time.
  if (unAuthScan == 3) {
    digitalWrite(RGBred, HIGH);
    delay(lockout[0]);
    digitalWrite(RGBred, LOW);
  } else if (unAuthScan == 4) {
    digitalWrite(RGBred, HIGH);
    delay(lockout[1]);
    digitalWrite(RGBred, LOW);
  } else if (unAuthScan == 5) {
    digitalWrite(RGBred, HIGH);
    delay(lockout[2]);
    digitalWrite(RGBred, LOW);    
  } else if (unAuthScan >= 6) {
    digitalWrite(RGBred, HIGH);
    delay(lockout[3]);
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

  // Flash led / beep a set number of times.
  flashBeep(authorisedStates, interval, RGBgreenState, RGBgreen);

  // Play the track assigned when a authorised card / implant is scanned.
  #if defined (usingmp3player)
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

// Check read card against authorised uid's
bool checkcard(byte readCard[]){
  #ifdef hardcodeUID
    for (unsigned int i = 0; i < numberofitems(uids); i++){
      if (memcmp(uids[i], readCard, sizeof(uids[i])) == 0){
        return true;
      }
    }
    return false;
  #endif

  #ifndef hardcodeUID
    for (int i = 0; i < authorisedCards.size(); i++){
      card curentCard = authorisedCards.get(i);
      int size = sizeof(readCard);
      if (memcmp(curentCard.uid, readCard, size) == 0){
        return true;
      }
    }
    return false;
  #endif
}

// Attempts to read a card from the RC522.
#ifdef usingRC522
  bool rc522Read(){
    // On each loop check if a ISO14443A card has been found.
      if ( ! mfrc522.PICC_IsNewCardPresent()) 
      {
        return false;
      }
      // Select one of the cards
      if ( ! mfrc522.PICC_ReadCardSerial()) 
      {
        return false;
      }

      
      for (byte i = 0; i < mfrc522.uid.size; i++) 
      {
        readCard[i] = mfrc522.uid.uidByte[i];
      }
      return true;
}
#endif

// Attempts to read a card from the PN532.
#ifdef usingPN532
  bool pn532Read(){
    bool success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
    // On each loop check if a ISO14443A card has been found.
    success = pn532.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

    if (success) {
      #ifdef debug
        Serial.println(F("Found a card!"));
      #endif

      for (uint8_t i=0; i < uidLength; i++) {
        readCard[i] = uid[i];
      }
      return true;
    } else {
      return false;
    }    
  }
#endif

// Blocks until a card is present at the reader.
void waitForCard(){ 
    cardRead = false;
    while (!cardRead) {
      #ifdef usingRC522
        cardRead = rc522Read();
      #endif

      #ifdef usingPN532
        cardRead = pn532Read();        
      #endif
    }
}

// Clear meathods when using master cards.
#ifndef hardcodeUID
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

    // Flash led / beep a set number of times.
    flashBeep(5, interval, RGBblueState, RGBblue);

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
    waitForCard();

    // Read Master Card's UID from EEPROM
    for ( int i = 0; i < masterSize; i++ ) {          
      masterCard[i] = EEPROM.read(masterUIDSizeLocation + 1 + i);
    }

    #ifdef debug
      PrintHex8(masterCard, masterSize); // Print each character of the mastercard UID.
      Serial.println("");
    #endif

    // See if scanned card is the master.
    if (checkmaster(readCard)) {
      #ifdef debug
        Serial.println(F("-------------------"));
        Serial.println(F("This is your last chance"));
        Serial.println(F("You have 10 seconds to Cancel"));
        Serial.println(F("This will be remove all records and cannot be undone"));
      #endif

      // Flash led / beep a set number of times.
      flashBeep(1, interval, RGBredState, RGBred);      
      flashBeep(1, interval, RGBgreenState, RGBgreen);
      flashBeep(1, interval, RGBredState, RGBred);

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
#endif

//clear card read arrays and reset readstate booleans:
void cleanup(){
    cardRead = false;
    matchfound = false;

    // Clear readCard array.
    for (int i = 0; i < 7; i++) {
      readCard[i] = 00;
    }

    for (int i = 0; i < 4; i++) {
      smallCard[i] = 00;
    }
}

// Setup.
void setup() {
  #ifdef usingLED
    pinMode(RGBred, OUTPUT);   // Declaring red LED as an output.
    digitalWrite(RGBred, LOW); // Setting it to OFF.
    pinMode(RGBgreen, OUTPUT);   // Declaring green LED as an output.
    digitalWrite(RGBgreen, LOW); // Setting it to OFF.
    pinMode(RGBblue, OUTPUT);   // Declaring blue LED as an output.
    digitalWrite(RGBblue, LOW); // Setting it to OFF.

    // Quick little flash 
    digitalWrite(RGBgreen, HIGH); // Activates green led.
    delay(500); // Waits 0.5 seconds.
    digitalWrite(RGBgreen, LOW); // Deactivates green led.
    delay(100); // Waits 0.1 seconds.
    digitalWrite(RGBblue, HIGH); // Activates blue led.
    delay(500); // Waits 0.5 seconds.
    digitalWrite(RGBblue, LOW); // Deactivates blue led.
    delay(100); // Waits 0.1 seconds.
    digitalWrite(RGBred, HIGH); // Activates red led.
    delay(500); // Waits 0.5 seconds.
    digitalWrite(RGBred, LOW); // Deactivates red led.

    // If the buzzer isnt being used.
    #ifndef usingBuzzer
      // Set the number of flashes and beeps to blinks.
      authorisedOutputs = authorisedBlinks;
      unauthorisedOutputs = unauthorisedBlinks;

      // Set the number of state changes required to output x times and turn back off.
      if ((authorisedOutputs % 2) == 0) {
        authorisedStates = authorisedOutputs + 1;
      } else {
        authorisedStates = authorisedOutputs + 2;
      }

      if ((unauthorisedOutputs % 2) == 0) {
        unauthorisedStates = unauthorisedOutputs + 1;
      } else {
        unauthorisedStates = unauthorisedOutputs + 2;
      }

      // Set the interval to blink interval.
      interval = RGBinterval;

    #endif
  #endif

  #ifdef usingBuzzer
    pinMode(Buzzer, OUTPUT);   // Declaring buzzer as an output.
    digitalWrite(Buzzer, LOW); // Setting it to OFF.

    // Quick little beep.
    digitalWrite(Buzzer, HIGH); // Activates the buzzer.
    delay(200); // Delay 0.2 seconds.
    digitalWrite(Buzzer, LOW); // Deactivates the buzzer.

    // If the LED isnt being used.
    #ifndef usingLED
      // Set the number of flashes and beeps to blinks.
      authorisedOutputs = authorisedBuzzes;
      unauthorisedOutputs = unauthorisedBuzzes;

      // Set the number of state changes required to output x times and turn back off.
      if ((authorisedOutputs % 2) == 0) {
        authorisedStates = authorisedOutputs + 1;
      } else {
        authorisedStates = authorisedOutputs + 2;
      }

      if ((unauthorisedOutputs % 2) == 0) {
        unauthorisedStates = unauthorisedOutputs + 1;
      } else {
        unauthorisedStates = unauthorisedOutputs + 2;
      }

      // Set the interval to blink interval.
      interval = Buzzerinterval;

    #endif
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

      // Error state outputs (buzzer beep).
      #ifdef usingBuzzer
        digitalWrite(Buzzer, HIGH); // Activates the buzzer.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(Buzzer, LOW); // Deactivates the buzzer.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(Buzzer, HIGH); // Activates the buzzer.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(Buzzer, LOW); // Deactivates the buzzer.
      #endif
      
      // Error state outputs (Red led flash).
      #ifdef usingLED
        digitalWrite(RGBred, HIGH); // Activates the red LED.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(RGBred, LOW); // Deactivates the red LED.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(RGBred, HIGH); // Activates the red LED.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(RGBred, LOW); // Deactivates the red LED.
      #endif

      // Halt.
      while(true){}
    }

    #ifdef debug
      Serial.println(F("mp3Player ready."));
    #endif

    myDFPlayer.volume(volume);  //Set volume value. From 0 to 30
    myDFPlayer.play(startupTrack);  // Play the second mp3 file.
  #endif

  #ifdef debug
    Serial.begin(115200);   // Initiate a serial communication at 115200 baud.
    while (!Serial) {}  // Wait for serial to be open.
  #endif

  #ifdef usingRC522
    SPI.begin();  // Initiate  SPI bus.
    delay(2000); // Wait for the SPI to come up.
    mfrc522.PCD_Init(); // Initiate MFRC522
    mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max); // Set antenna gain to max (longest range).
    byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg); // Get the software version.

    if ((v == 0x00) || (v == 0xFF)) {
      #ifdef debug
        Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
        Serial.println(F("SYSTEM HALTED: Check connections."));
      #endif

      // Error state outputs (buzzer beep).
      #ifdef usingBuzzer
        digitalWrite(Buzzer, HIGH); // Activates the buzzer.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(Buzzer, LOW); // Deactivates the buzzer.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(Buzzer, HIGH); // Activates the buzzer.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(Buzzer, LOW); // Deactivates the buzzer.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(Buzzer, HIGH); // Activates the buzzer.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(Buzzer, LOW); // Deactivates the buzzer.
      #endif
      
      // Error state outputs (Red led flash).
      #ifdef usingLED
        digitalWrite(RGBred, HIGH); // Activates the red LED.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(RGBred, LOW); // Deactivates the red LED.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(RGBred, HIGH); // Activates the red LED.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(RGBred, LOW); // Deactivates the red LED.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(RGBred, HIGH); // Activates the red LED.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(RGBred, LOW); // Deactivates the red LED.
      #endif

      // Halt.
      while (true);
    }

    // Reader is ready.
    #ifdef debug
      Serial.println(F("Reader is ready."));
    #endif
  #endif

  #ifdef usingPN532
    pn532.begin();
    uint32_t versiondata = pn532.getFirmwareVersion();

    // Check the device is connected, error if not.
    if (!versiondata) {
      // Error state outputs (debug messages).
      #ifdef debug
        Serial.print("Didn't find PN53x board");
      #endif

      // Error state outputs (buzzer beep).
      #ifdef usingBuzzer
        digitalWrite(Buzzer, HIGH); // Activates the buzzer.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(Buzzer, LOW); // Deactivates the buzzer.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(Buzzer, HIGH); // Activates the buzzer.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(Buzzer, LOW); // Deactivates the buzzer.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(Buzzer, HIGH); // Activates the buzzer.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(Buzzer, LOW); // Deactivates the buzzer.
      #endif
      
      // Error state outputs (Red led flash).
      #ifdef usingLED
        digitalWrite(RGBred, HIGH); // Activates the red LED.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(RGBred, LOW); // Deactivates the red LED.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(RGBred, HIGH); // Activates the red LED.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(RGBred, LOW); // Deactivates the red LED.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(RGBred, HIGH); // Activates the red LED.
        delay(200); // Delay 0.2 seconds.
        digitalWrite(RGBred, LOW); // Deactivates the red LED.
      #endif

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
    pn532.setPassiveActivationRetries(0xFF);

    // configure board to read RFID tags
    pn532.SAMConfig();

    // Reader is ready.
    #ifdef debug
      Serial.println(F("Reader is ready."));
    #endif      
  #endif

  #ifndef hardcodeUID
    // Setup the clear button for clearing eeprom.
    pinMode(clearButton, INPUT);
    digitalWrite(clearButton, HIGH);

    // Check if button is pressed.
    if (digitalRead(clearButton) == LOW) {
      scorchedEarth = true;
    }
  #endif

  #if defined(usingLED) && defined(usingBuzzer)
    // Set the number of flashes and beeps to the greater of the 2 inputs.
    if (authorisedBlinks >= authorisedBuzzes) {
      authorisedOutputs = authorisedBlinks;
    } else {
      authorisedOutputs = authorisedBuzzes;
    }

    if (unauthorisedBlinks >= unauthorisedBuzzes) {
      unauthorisedOutputs = unauthorisedBlinks;
    } else {
      unauthorisedOutputs = unauthorisedBuzzes;
    }

    // Set the number of state changes required to output x times and turn back off.
    if ((authorisedOutputs % 2) == 0) {
      authorisedStates = authorisedOutputs + 1;
    } else {
      authorisedStates = authorisedOutputs + 2;
    }

    if ((unauthorisedOutputs % 2) == 0) {
      unauthorisedStates = unauthorisedOutputs + 1;
    } else {
      unauthorisedStates = unauthorisedOutputs + 2;
    }


    // Set the interval to which ever is greater.
    if (RGBinterval >= Buzzerinterval) {
      interval = RGBinterval;
    } else {
      interval = Buzzerinterval;
    }

  #endif

  #ifdef sleep
    pinMode(touchPin, INPUT);
  #endif
}

// Main loop.
void loop() {

  #ifndef hardcodeUID
    
    // Read all values from eeprom, only once needed.
    if (!eepromRead) {

      // Check to see if we are entering clearing mode.
      if (scorchedEarth == true) {
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

        // Flash led / beep a set number of times.
        flashBeep(3, interval, RGBredState, RGBred);
        flashBeep(1, interval, RGBgreenState, RGBgreen);

        // Wait until we scan a card befor continuing.
        waitForCard();

        // Check to see if 4 or 7 byte uid.
        itsA4byte = check4Byte();

        // Write the scanned card to the eeprom as the master card.
        if (!itsA4byte) {
          EEPROM.update(masterUIDSizeLocation, 7);
          masterSize = 7;
          for (unsigned int i = 0; i < 7; i++ ) {        
            EEPROM.update(masterUIDSizeLocation + 1 + i, readCard[i]);  // Write scanned PICC's UID to EEPROM, start from address 3
          }
        } else {
          masterSize = 4;
          EEPROM.update(masterUIDSizeLocation, 4);
          for (unsigned int i = 0; i < 4; i++ ) {        
            EEPROM.update(masterUIDSizeLocation + 1 + i, smallCard[i]);  // Write scanned PICC's UID to EEPROM, start from address 3
          }
        }

        // Print the mastercard UID.
        #ifdef debug
          if (!itsA4byte) {
              PrintHex8(readCard, 7); 
              Serial.println("");
          } else {
            PrintHex8(smallCard, 4); 
            Serial.println("");
          }
        #endif

        // Write to EEPROM we defined Master Card.
        EEPROM.update(masterDefinedLocation, 143);

      }

      // Read the number of cards currently stored, not including master.
      cardCount = EEPROM.read(cardCountLocation);

      // Read the master card and place in memory.
      masterSize = EEPROM.read(masterUIDSizeLocation);

      for (int i = 0; i < masterSize; i++) {
        byte recievedByte = readUIDbit(masterUIDSizeLocation + 1, i);
        memcpy(masterCard + i, &recievedByte, 1);
      }

      // Read access cards and place in memory.
      if (cardCount != 0) {
        // Itterate through the number of cards.
        int startPos = masterUIDSizeLocation + masterSize + 1;
        
        for (int i = 0; i < cardCount; i++) {
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
          authorisedCards.add(currentCard);

          startPos = startPos + size;

          // Print out card details.
          #ifdef debug
            Serial.println(F("-------------------"));
            Serial.print(F("Card number: "));
            Serial.println(currentCard.index);
            Serial.print(F("Card size in bytes: "));
            Serial.println(currentCard.size);
            Serial.print(F("Card uid: "));
            PrintHex8(currentCard.uid, currentCard.size);
            Serial.println(F(""));
          #endif
        }
        // Output to show number of access cards defined.
        #ifdef debug
          Serial.println(F("-------------------"));          
          Serial.print(F("There are "));
          Serial.print(cardCount);
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

        // Flash led / beep a set number of times.
        flashBeep(3, interval, RGBredState, RGBred);
        flashBeep(1, interval, RGBblueState, RGBblue);
      }

      // Clear readCard arrays.
      for (int i = 0; i < 7; i++) {
        readCard[i] = 00;
      }

      for (int i = 0; i < 4; i++) {
        smallCard[i] = 00;
      }

      eepromRead = true;
    }
  #endif
  
  #ifdef usingRC522
    cardRead = rc522Read();
  #endif

  #ifdef usingPN532
    cardRead = pn532Read();        
  #endif

  #ifndef hardcodeUID
    if (cardRead) {
      itsA4byte = check4Byte();  
    }
  #endif

  #ifdef hardcodeUID
    // match found flag.
    bool matchFound = false;

    // Check whether the card read is known.
    if (cardRead) {
      matchFound = checkcard(readCard);
    }
  #endif

  #ifndef hardcodeUID
    // master found flag.
    bool masterFound = false;

    // match found flag.
    bool matchFound = false;

    // Check whether the card read is master or even known.
    if (cardRead) {
      if (!itsA4byte) {
        masterFound = checkmaster(readCard);
        matchFound = checkcard(readCard);
      } else {
        masterFound = checkmaster(smallCard);
        matchFound = checkcard(smallCard);
      }
    }

    // Learning mode.
    if (cardRead && masterFound) {
      // Learning mode flag.
      bool learningMode = true;

      // Output to show entered learing mode.
      #ifdef debug
        Serial.println(F("-------------------"));
        Serial.println(F("Master scanned."));
        Serial.println(F("Scan access card to ADD or REMOVE."));
      #endif 

      #ifdef usingLED
        // Flash led / beep a set number of times.
        flashBeep(3, interval, RGBgreenState, RGBgreen);
        flashBeep(1, interval, RGBblueState, RGBblue);
      #endif

      // Clear readCard arrays.
      for (int i = 0; i < 7; i++) {
        readCard[i] = 00;
      }

      for (int i = 0; i < 4; i++) {
        smallCard[i] = 00;
      }

      while (learningMode){
        // Wait for a card to be scanned.
        waitForCard();
        
        // Check to see if card is a 4 byte UID.
        itsA4byte = check4Byte();

        // Check if scanned card was master. 
        masterFound = false;
        bool cardDefined = false;
        
        if (!itsA4byte) {
          masterFound = checkmaster(readCard);
        } else {
          masterFound = checkmaster(smallCard);
        }

        // If the card was master the leave learning mode.
        if (masterFound){
          // Output to show leaving learing mode.
          #ifdef debug
            Serial.println(F("-------------------"));
            Serial.println(F("Master scanned."));
            Serial.println(F("Exiting learning mode."));
          #endif 

          #ifdef usingLED
            // Flash led / beep a set number of times.
            flashBeep(3, interval, RGBgreenState, RGBgreen);
            flashBeep(1, interval, RGBblueState, RGBblue);
          #endif

          learningMode = false;

          // Clear readCard array.
          cleanup();

          // Reset sleep timer.
          startMillis = millis();

          return;
        } else {
          // Card was not master, is it defined?
          if (!itsA4byte) {
            cardDefined = checkcard(readCard);
          } else {
            cardDefined = checkcard(smallCard);
          }
        }

        // Has the card already been defined.
        if (!cardDefined) {
          // Print message to say it will be added to memory.
          #ifdef debug
            if (!itsA4byte){
              Serial.println(F("-------------------"));
              Serial.println(F("Scanned card had a UID of."));
              PrintHex8(readCard, 7);
              Serial.println("");
              Serial.println(F("It will be added to memory."));
            } else {
              Serial.println(F("-------------------"));
              Serial.println(F("Scanned card had a UID of."));
              PrintHex8(smallCard, 4);
              Serial.println("");
              Serial.println(F("It will be added to memory."));
            }
          #endif

          // Create a start position.
          int startPos = masterUIDSizeLocation + masterSize + 1;

          // Find the next place to write the card.
          for (int i = 0; i < authorisedCards.size(); i++) {
            card myCard = authorisedCards.get(i);
            startPos = startPos + myCard.size + 1;
          }

          if (!itsA4byte){ 
            // Create a card instance for the scanned card.
            card currentCard;
            
            for (int j = 0; j < 7; j++) {
              memcpy(currentCard.uid + j, readCard + j, 1);
            }

            currentCard.size = 7;
            currentCard.index = cardCount;            
            
            // Write uid size in bytes.
            EEPROM.update(startPos, currentCard.size);
            startPos++;

            // Write uid of card to eeprom.
            for (int i = 0; i < 7; i++) {
              EEPROM.update(startPos + i, readCard[i]);
            }

            // add the scanned card to the list
            authorisedCards.add(currentCard);

          } else {
            // Create a card instance for the scanned card.
            card currentCard;
            
            for (int j = 0; j < 4; j++) {
              memcpy(currentCard.uid + j, readCard + j, 1);
            }

            currentCard.size = 4;
            currentCard.index = cardCount;
          
            // Write uid size in bytes.
            EEPROM.update(startPos, currentCard.size);
            startPos++;

            // Write uid of card to eeprom.
            for (int i = 0; i < 4; i++) {
              EEPROM.update(startPos + i, smallCard[i]);
            }

            // add the scanned card to the list
            authorisedCards.add(currentCard);
          }

          // Update the number of known cards.
          cardCount++;
          EEPROM.update(cardCountLocation, cardCount);

          #ifdef usingLED
            // Flash led / beep a set number of times.
            flashBeep(3, interval, RGBblueState, RGBblue);
            flashBeep(1, interval, RGBgreenState, RGBgreen);
          #endif

          // Gives the user a second to pull the card away.
          delay(1000);

        }
        
        if (cardDefined) {
          // Print card is defined.
          #ifdef debug
            if (!itsA4byte){
              Serial.println(F("-------------------"));
              Serial.println(F("Scanned card had a UID of."));
              PrintHex8(readCard, 7);
              Serial.println("");
              Serial.println(F("It will be removed from memory."));
            } else {
              Serial.println(F("-------------------"));
              Serial.println(F("Scanned card had a UID of."));
              PrintHex8(smallCard, 4);
              Serial.println("");
              Serial.println(F("It will be removed from memory."));
            }
          #endif

          // Find where in the list the card to be removed is.
          card currentCard;
          if (!itsA4byte) {          
            currentCard = findCardtoRemove(readCard);
          } else {
            currentCard = findCardtoRemove(smallCard);
          }

          // Check to see if the current card is the errorUID (ERROR!!).
          if (memcmp(currentCard.uid, errorUID, 7) == 0) {
            #ifdef debug
              if (!itsA4byte){
              Serial.println(F("-------------------"));
              Serial.println(F("Scanned card had a UID of."));
              PrintHex8(readCard, 7);
              Serial.println("");
              Serial.println(F("It was not found."));
            } else {
              Serial.println(F("-------------------"));
              Serial.println(F("Scanned card had a UID of."));
              PrintHex8(smallCard, 4);
              Serial.println("");
              Serial.println(F("It was not found."));
            }
            #endif
            
            #ifdef usingLED
              // Flash led / beep a set number of times.
              flashBeep(1, interval, RGBredState, RGBred);
              flashBeep(1, interval, RGBblueState, RGBblue);
              flashBeep(1, interval, RGBredState, RGBred);
            #endif

            return;            
          }

          // Remove the card from the linked list.
          authorisedCards.remove(currentCard.index);

          // Go through the eeprom and remove the blank space.
          int startPos = masterUIDSizeLocation + masterSize + 1;
          for (int i = 0; i < authorisedCards.size(); i++) {
            card myCard = authorisedCards.get(i);
            EEPROM.update(startPos, myCard.size);
            startPos++;

            for (int j = 0; j < myCard.size; j++) {
              EEPROM.update(startPos + j, myCard.uid[j]);
            }

            startPos = startPos + myCard.size;

          }

          // Update the number of known cards.
          cardCount--;
          EEPROM.update(cardCountLocation, cardCount);

          #ifdef usingLED  
            // Flash led / beep a set number of times.
            flashBeep(3, interval, RGBblueState, RGBblue);
            flashBeep(1, interval, RGBredState, RGBred);
          #endif

          // Gives the user a second to pull the card away.
          delay(1000);
        }

        // Reset sleep timer.
        startMillis = millis();

        // Clear readCard arrays.
        cleanup();
      }
    }

  #endif

  // If a match is found and the device is not already in an enabled state activate.
  if (matchFound && cardRead && !enabled) {
    authorised();
    cleanup();
  }

  // If a match is found and the device is already in an enabled state shutdown.
  if (matchFound && cardRead && enabled ) {
    shutdown();
    cleanup();
  }

  // If a match is not found unauthorised.
  if (!matchFound && cardRead) {
    unauthorised();
    cleanup();
   }

  #ifdef sleep
    // Should we sleep yet?
    currentMillis = millis();
    if(currentMillis >= startMillis + timeToWait) {
      sleepNow();
    }
  #endif
}