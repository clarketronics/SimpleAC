#include "Authorised.h"

// If UID is correct do this.
void authorised(Data &data, FlashBeep &feedback) { 
    // Reset the un authorised scan counter.
    EEPROM.update(unauthScanCountLocation, 0);

    // Serial message to notify that UID is valid.
    #ifdef debug
    Serial.println(F("Access authorised."));
    #endif

    // Flash green led and beep a 3 times.
    feedback.output(SHORT_PERIOD, 3, RGBgreen);

    // Play the track assigned when a authorised card / implant is scanned.
    #ifdef using_MP3
    mp3Play(DFPlayer, authorisedTrack);
    #endif

    // Check what state the input is in.
    inputState State = digitalRead(statePin) ? Locked : Unlocked;

    // Trigger relay one or two.
    switch (State){
        case Locked:
            digitalWrite(relay1, HIGH); // Turn on relay 1.

            #ifdef debug
                Serial.println(F("Relay one engaged."));
            #endif

            delay(unlockDelay); //delay x seconds.

            digitalWrite(relay1, LOW); // Turn off relay 1.

            #ifdef debug
                Serial.println(F("Relay one disengaged."));
            #endif    

        break;
        case Unlocked:
            digitalWrite(relay2, HIGH); // Turn on relay 2.

            #ifdef debug
                Serial.println(F("Relay two engaged."));
            #endif

            delay(unlockDelay); //delay x seconds.

            digitalWrite(relay2, LOW); // Turn off relay 2.

            #ifdef debug
                Serial.println(F("Relay two disengaged."));
            #endif            
        break;
    }    

    #ifdef Sleep
        data.startMillis = millis(); // Reset timeout counter.
    #endif
}