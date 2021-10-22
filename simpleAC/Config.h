//-------Module Definitions- Delete the "//" of the modules you are using----------
//#define debug // Unmark this to enable output printing to the serial monitor, this will not continue without a open serial port.
//#define using_MP3 // Unmark this if using the df mp3player.
//#define using_RC522 // Unmark this if you are using the RC522 13.56MHz NFC-HF RFID reader.
#define using_PN532 // Unmark this if you are using the PN532 13.56MHz NFC-HF RFID reader.
#define using_LED // Unmark this if you want to enable the RGB LED.
#define using_Buzzer // Unmark this if you want to enable the Buzzer.
#define Sleep // Unmark this if you want to enable sleep mode (conserves battery).

//-------Mode Definitions- Delete the "//" of the mode you want to use----------
#define car // This mode is designed to lock and unlock a car.
//#define bike // This mode is designed to prime and then starts the bike.
//#define accessory // This mode is designed to give a latching output when authorised.


//------Car mode variables------
#ifdef car
    #define statePin 2 // State input pin, locked / unlocked?
    #define unlockDelay 500 // Amount of time to wait before turning the relay back off.
    #define lockDelay 500 // Amount of time to wait before turning the relay back off.
#endif

//------Bike mode variables------
#ifdef bike
    #define primeTime 3000 // The amount of time after relay 1 triggers to wait for the bike to prime.
    #define startTime 5000 // The amount of time to run the starter motor.
#endif


// Don't touch anything below, or do im not your mum.

//------Relay setup------
#define relay1 5 // Relay 1.
#define relay2 6 // Relay 2.

//------Default input state------
#define defaultState HIGH // Default state for inputs.

//------Sleep variables------
#define wakeupPin 3 // The pin that will wake up the device from sleep, by defualt pin 3. This must be an interupt pin.
#define triggerState HIGH // What state should we look for when sleeping.
#define timeToWait  5000 // The amount of time in millis to wait before sleeping.

//------MP3 variables------
#define Volume 30 // Set the volume of the mp3 played 0~30.
#define startupTrack 001 // Track played when MP3 player boots.
#define authorisedTrack 002 // Track played when an autorised card / implant is scanned.
#define unauthorisedTrack 003 // Track played when an autorised card / implant is scanned.
