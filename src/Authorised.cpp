#include "Authorised.h"

// Car authorised routine
#ifdef car
    // If UID is correct do this.
    void authorised(Data &data, FlashBeep &feedback) { 
        // Reset the un authorised scan counter.
        EEPROM.update(unauthScanCountLocation, 0);

        // Serial message to notify that UID is valid.
        #ifdef debug
            Serial.println(F("-------------------"));
            Serial.println(F("Access authorised."));
        #endif

        // Play the track assigned when a authorised card / implant is scanned.
        #ifdef using_MP3
            mp3Play(DFPlayer, authorisedTrack);
        #endif

        // Check what state the input is in.
        inputState State = digitalRead(statePin) ? Locked : Unlocked;

        // Trigger relay one or two.
        switch (State){
            case Locked:
                // Flash green, blue and green plus beep a 3 times.
                feedback.output(SHORT_PERIOD, 1, RGBgreen);
                feedback.output(SHORT_PERIOD, 1, RGBblue);
                feedback.output(SHORT_PERIOD, 1, RGBgreen);

                digitalWrite(relay1, HIGH); // Turn on relay 1.
                
                #ifdef debug
                    Serial.println(F("-------------------"));
                    Serial.println(F("Unlocking."));
                #endif

                delay(unlockDelay); //delay x seconds.

                digitalWrite(relay1, LOW); // Turn off relay 1.

                if (digitalRead(statePin) ? Locked : Unlocked != State){
                    #ifdef debug
                        Serial.println(F("-------------------"));
                        Serial.println(F("Unlocked."));
                    #endif
                } else {
                    // Flash red, blue and green plus beep a 3 times.
                    feedback.output(SHORT_PERIOD, 1, RGBred);
                    feedback.output(SHORT_PERIOD, 2, RGBblue);

                    #ifdef debug
                        Serial.println(F("-------------------"));
                        Serial.println(F("Unlocking failed"));
                    #endif
                }    

            break;
            case Unlocked:
                // Flash green, blue and blue plus beep a 3 times.
                feedback.output(SHORT_PERIOD, 1, RGBgreen);
                feedback.output(SHORT_PERIOD, 2, RGBblue);
                
                digitalWrite(relay2, HIGH); // Turn on relay 2.

                #ifdef debug
                    Serial.println(F("Locking"));
                #endif

                delay(unlockDelay); //delay x seconds.

                digitalWrite(relay2, LOW); // Turn off relay 2.

                if (digitalRead(statePin) ? Locked : Unlocked != State){
                    #ifdef debug
                        Serial.println(F("-------------------"));
                        Serial.println(F("Locked."));
                    #endif
                } else {
                    // Flash blue, red and red plus beep a 3 times.
                    feedback.output(SHORT_PERIOD, 1, RGBblue);
                    feedback.output(SHORT_PERIOD, 2, RGBred);

                    #ifdef debug
                        Serial.println(F("-------------------"));
                        Serial.println(F("Locking failed"));
                    #endif
                }           
            break;
        }    

        #ifdef Sleep
            data.startMillis = millis(); // Reset timeout counter.
        #endif
    }
#endif

// Bike authorised routine
#ifdef bike
    // If UID is correct do this.
    void authorised(Data &data, FlashBeep &feedback) { 
        // Reset the un authorised scan counter.
        EEPROM.update(unauthScanCountLocation, 0);

        // Serial message to notify that UID is valid.
        #ifdef debug
            Serial.println(F("-------------------"));
            Serial.println(F("Access authorised."));
        #endif

        // Flash green led and beep a 3 times.
        feedback.output(SHORT_PERIOD, 3, RGBgreen);

        // Play the track assigned when a authorised card / implant is scanned.
        #ifdef using_MP3
            mp3Play(DFPlayer, authorisedTrack);
        #endif


        digitalWrite(relay1, HIGH); // Turn on relay 1, main power to the bike.

        // Serial message to say were priming.
        #ifdef debug
            Serial.println(F("-------------------"));
            Serial.println(F("Priming."));
        #endif

        delay(primeTime); // Wait the amount of time for the bike to prime.

        digitalWrite(relay2, HIGH); // Turn on relay 2, same as pushing the start button.

        // Serial message to say were startin.
        #ifdef debug
            Serial.println(F("-------------------"));
            Serial.println(F("Starting."));
        #endif

        delay(startTime); // Run starter motor for the alloted time.  

        #ifdef Sleep
            data.startMillis = millis(); // Reset timeout counter.
        #endif
    }
#endif

// Accessory authorised routine
#ifdef accessory
    // If UID is correct do this.
    void authorised(Data &data, FlashBeep &feedback) { 
        // Reset the un authorised scan counter.
        EEPROM.update(unauthScanCountLocation, 0);

        // Serial message to notify that UID is valid.
        #ifdef debug
            Serial.println(F("-------------------"));
            Serial.println(F("Authorised."));
        #endif

        // Flash green led and beep a 3 times.
        feedback.output(SHORT_PERIOD, 3, RGBgreen);

        // Play the track assigned when a authorised card / implant is scanned.
        #ifdef using_MP3
            mp3Play(DFPlayer, authorisedTrack);
        #endif

        // If the relay is not enabled turn on else turn off.
        if (!data.enabled){
            digitalWrite(relay1, HIGH); // Turn on relay 1.
        } else {
            digitalWrite(relay1, LOW); // Turn off relay 1.
        }

        // Toggle enabled state.
        data.enabled = !data.enabled;

        #ifdef Sleep
            data.startMillis = millis(); // Reset timeout counter.
        #endif
    }
#endif