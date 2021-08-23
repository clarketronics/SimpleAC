#include "MP3.h"

// Startup the mp3 player.
void mp3Begin(DFRobotDFPlayerMini &mp3, SoftwareSerial &softSerial, FlashBeep &feedback){
    softSerial.begin(9600); // Start SoftwareSerial at 9600 baud.

    // Check the device is connected, error if not.
    if (!mp3.begin(softSerial)) { 
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

    mp3.volume(Volume);  //Set volume value. From 0 to 30
    mp3.play(startupTrack);  // Play the second mp3 file.
}

// Play a track
void mp3Play(DFRobotDFPlayerMini &mp3, int track){
    mp3.play(track);
}

