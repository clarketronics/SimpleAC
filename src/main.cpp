/* NOTES: 
  * If LED's and Buzzer are enabled the authorised and unautorised blinks / buzzes 
    will be which ever value is greater.

  * If LED's and Buzzer are enabled the interval between blinks / buzzes 
    will be which ever value is greater.

  * Pull LT pin (D2) LOW at startup and you will enter a clear mode
    (this is shown by serial messages, 3 blue LED flashes and beeps) to clear the eeprom.
*/

//-------Definitions- Delete the "//" of the modules you are using----------
#define debug // Unmark this to enable output printing to the serial monitor, note this will not continue without a open serial port.
//#define usingMP3 // Unmark this if using the df mp3player.
//#define usingRC522 // Unmark this if you are using the RC522 13.56MHz NFC-HF RFID reader.
#define usingPN532 // Unmark this if you are using the RC522 13.56MHz NFC-HF RFID reader.
#define usingLED // Unmark this if you want to enable the RGB LED.
#define usingBuzzer // Unmark this if you want to enable the Buzzer.
#define usingRelays // Unmark this if you want to enable the Relays.
//#define sleep // Unmark this if you want to enable sleep mode (conserves battery).
//#define state // Unmark this if your using a micro switch to detect the state of the lock.

//-------Global variables and includes-------
#include <Arduino.h>
#include <EEPROM.h>
#include "FlashBeep.h"
#include "Helpers.h"
#define unauthScanCountLocation 2
bool enabled;

// Catch idiots defining both reader.
#if defined(usingRC522) && defined(usingPN532) 
#error "You can't define both readers at the same time!."
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

class card {
  public:
    byte uid[7];
    int size;
    int index;
};

LinkedList<card> authorisedCards = LinkedList<card>();

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

    state = waitingOnCard;

    #ifdef usingRC522
      mfrc522.PCD_SoftPowerUp(); // Turn the reader back on.
    #endif

    detachInterrupt(0); // Disables the interrupt.

    startMillis = millis(); // Set the last start up time.
  }
  
#endif

// Print hex messages with leading 0's
void PrintHex8(uint8_t *data, uint8_t length) { 
  for (int i=0; i<length; i++) { 
    if (data[i]<0x10) {Serial.print("0");} 
    Serial.print(data[i],HEX); 
    Serial.print(" "); 
  }
}

// Create instance of custom libraries.
FlashBeep feedback;
Helpers helpers;

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
      feedback.flashBeep(SHORT_PERIOD, 2, RGBred);

      // Halt.
      while(true){}
    }

    #ifdef debug
      Serial.println(F("mp3Player ready."));
    #endif

    myDFPlayer.volume(volume);  //Set volume value. From 0 to 30
    myDFPlayer.play(startupTrack);  // Play the second mp3 file.
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

      // Flash the red LED and beep 3 times.
      feedback.flashBeep(SHORT_PERIOD, 3, RGBred, RGBredState);

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

      // Flash the red LED and beep 3 times.
      feedback.flashBeep(SHORT_PERIOD, 3, RGBred);

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
  feedback.flashBeep(SHORT_PERIOD, 3, RGBgreen);


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
  feedback.flashBeep(SHORT_PERIOD, 3, RGBred);

  // Play the track assigned when a unauthorised card / implant is scanned.
  #ifdef usingmp3player
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

  // Flash green led and beep a 3 times.
  feedback.flashBeep(SHORT_PERIOD, 3, RGBgreen);

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

// Check read card against authorised uid's
bool checkCard(byte readCard[]){
    for (int i = 0; i < authorisedCards.size(); i++){
      card curentCard = authorisedCards.get(i);
      int size = sizeof(readCard);
      if (memcmp(curentCard.uid, readCard, size) == 0){
        return true;
      }
    }
    return false;
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
    bool cardRead = false;
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
  feedback.flashBeep(SHORT_PERIOD, 3, RGBblue);

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

    // Flash red, green, red led and beep a 3 times.
    feedback.flashBeep(SHORT_PERIOD, 1, RGBred);
    feedback.flashBeep(SHORT_PERIOD, 1, RGBgreen);
    feedback.flashBeep(SHORT_PERIOD, 1, RGBred);


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

//clear card read arrays and reset readstate booleans:
void cleanup(){
    // Clear readCard array.
    for (int i = 0; i < 7; i++) {
      readCard[i] = 00;
    }

    for (int i = 0; i < 4; i++) {
      smallCard[i] = 00;
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
    feedback.flashBeep(SHORT_PERIOD, 2, RGBred);
    feedback.flashBeep(SHORT_PERIOD, 1, RGBgreen);

    // Wait until we scan a card befor continuing.
    waitForCard();

    // Check to see if 4 or 7 byte uid.
    itsA4byte = helpers.check4Byte(readCard, smallCard);

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

    // Make sure user has moved card.
    delay(500);

  }

  // Read the number of cards currently stored, not including master.
  cardCount = EEPROM.read(cardCountLocation);

  // Read the master card and place in memory.
  masterSize = EEPROM.read(masterUIDSizeLocation);

  for (int i = 0; i < masterSize; i++) {
    byte recievedByte = helpers.readUIDbit(masterUIDSizeLocation + 1, i);
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

    // Flash red, red, blue and buzz with each flash.
    feedback.flashBeep(SHORT_PERIOD, 2, RGBred);
    feedback.flashBeep(SHORT_PERIOD, 1, RGBblue);
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

// Learning mode (add and remove cards).
void learning(){
  bool learning = true;

  while (learning){
    // Wait for a card to be scanned.
    waitForCard();
    
    // Check if scanned card was master. 
    bool masterFound = false;
    bool cardDefined = false;
    
    if (!helpers.check4Byte(readCard, smallCard)) {
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

      // Flash the red LED and beep 3 times.
      feedback.flashBeep(SHORT_PERIOD, 2, RGBgreen);
      feedback.flashBeep(SHORT_PERIOD, 1, RGBblue);

      // Clear readCard array.
      cleanup();

      #ifdef sleep
        // Reset sleep timer.
        startMillis = millis();
      #endif

      learning = false;
      state = waitingOnCard;

      delay(1000);

      return;
    } else {
      // Card was not master, is it defined?
      if (!helpers.check4Byte(readCard, smallCard)) {
        cardDefined = checkCard(readCard);
      } else {
        cardDefined = checkCard(smallCard);
      }

      // If the card is not defined add it to memory.
      if (!cardDefined) {
        // Print message to say it will be added to memory.
        #ifdef debug
          if (!helpers.check4Byte(readCard, smallCard)){
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

        if (!helpers.check4Byte(readCard, smallCard)){ 
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

        // Flash blue twice and red once, beep all 3 times.
        feedback.flashBeep(SHORT_PERIOD, 2, RGBblue);
        feedback.flashBeep(SHORT_PERIOD, 1, RGBgreen);

        // Gives the user a second to pull the card away.
        delay(1000);

      }

      // If the card is defined remove it from memory.
      if (cardDefined) {
        // Print card is defined.
        #ifdef debug
          if (!helpers.check4Byte(readCard, smallCard)){
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
        if (!helpers.check4Byte(readCard, smallCard)) {          
          currentCard = findCardtoRemove(readCard);
        } else {
          currentCard = findCardtoRemove(smallCard);
        }

        // Check to see if the current card is the errorUID (ERROR!!).
        if (memcmp(currentCard.uid, errorUID, 7) == 0) {
          #ifdef debug
            if (!helpers.check4Byte(readCard, smallCard)){
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

          // Flash red, blue then red, beep each time.
          feedback.flashBeep(SHORT_PERIOD, 1, RGBred);
          feedback.flashBeep(SHORT_PERIOD, 1, RGBblue);
          feedback.flashBeep(SHORT_PERIOD, 1, RGBred);
          
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

        // Flash blue twice red once, beep each time.
        feedback.flashBeep(SHORT_PERIOD, 2, RGBblue);
        feedback.flashBeep(SHORT_PERIOD, 2, RGBred);

        // Gives the user a second to pull the card away.
        delay(1000);
      }
    }
  }
}

// Main loop.
void loop() {
  // State machine!
  switch (state) {
    case startOfDay:
      onBoot();
      state = waitingOnCard;
      #ifdef sleep
        // Reset sleep timer.
        startMillis = millis();
      #endif
    break;
    case waitingOnCard:
      // Try to read a card.
      #ifdef usingRC522
        // Try and read a card.
        if (rc522Read()) {
          state = cardReadSuccessfully;
        } else {
          state = waitingOnCard;
        }
      #endif

      #ifdef usingPN532
        // Try and read a card.
        if (pn532Read()) {
          state = cardReadSuccessfully;
        } else {
          state = waitingOnCard;
        }        
      #endif
    break;
    case cardReadSuccessfully:
      // Check to see if card is 4 byte or not.
      if (helpers.check4Byte(readCard, smallCard)) {
        state = cardIs4Byte;
      } else {
        state = cardIs7Byte;
      }
    break;
    case cardIs4Byte:
      // Check whether the card read is master or even known.
      if (checkmaster(smallCard)) {
        state = cardIs4ByteMaster;
      } else if (checkCard(smallCard)) {
        state = cardIs4ByteAccess;
      } else {
        state = noMatch;
      }      
    break;
    case cardIs7Byte:
      // Check whether the card read is master or even known.
      if (checkmaster(readCard)) {
        state = cardIs7ByteMaster;
      } else if (checkCard(readCard)) {
        state = cardIs7ByteAccess;
      } else {
        state = noMatch;
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
      feedback.flashBeep(SHORT_PERIOD, 2, RGBgreen);
      feedback.flashBeep(SHORT_PERIOD, 1, RGBblue);

      // Clear read card arrays.
      cleanup();

      // Enter learning mode (this is blocking).
      learning();

      #ifdef sleep
        // Reset sleep timer.
        startMillis = millis();
      #endif

      // Clear readCard arrays.
      cleanup();
    break;
    case cardIs4ByteAccess:
    case cardIs7ByteAccess:
      // If a match is found and the device is not already in an enabled state activate.
      if (!enabled) {
        authorised();
        cleanup();
        state = waitingOnCard;
      } else {
        // If a match is found and the device is already in an enabled state shutdown.
        shutdown();
        cleanup();
        state = waitingOnCard;
      }
    break;
    case noMatch:
      // If a match is not found unauthorised.
      unauthorised();
      cleanup();
      state = waitingOnCard;
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
      state = goToSleep;
    }
  #endif
}