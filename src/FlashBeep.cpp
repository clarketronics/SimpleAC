#include "FlashBeep.h"

// Constructor ran when class is instantiated.
void FlashBeep::begin(bool BUZZER, bool LED) {
    // Setup LED if defined.
    if (LED) {
        // Set to make sure we know LED is enabled.
        RGB = true;
        // Pin declarations.
        pinMode(RGBred, OUTPUT);   // Declaring red LED as an output.
        digitalWrite(RGBred, LOW); // Setting it to OFF.
        pinMode(RGBgreen, OUTPUT);   // Declaring green LED as an output.
        digitalWrite(RGBgreen, LOW); // Setting it to OFF.
        pinMode(RGBblue, OUTPUT);   // Declaring blue LED as an output.
        digitalWrite(RGBblue, LOW); // Setting it to OFF.

        //Flash each LED once.
        digitalWrite(RGBgreen, HIGH); // Activates green led.
        delay(MEDIUM_PERIOD); // Waits 0.5 seconds.
        digitalWrite(RGBgreen, LOW); // Deactivates green led.
        digitalWrite(RGBblue, HIGH); // Activates blue led.
        delay(MEDIUM_PERIOD); // Waits 0.5 seconds.
        digitalWrite(RGBblue, LOW); // Deactivates blue led.
        digitalWrite(RGBred, HIGH); // Activates red led.
        delay(MEDIUM_PERIOD); // Waits 0.5 seconds.
        digitalWrite(RGBred, LOW); // Deactivates red led.
    } else {
        // Set to make sure we know LED is disabled.
        RGB = false;
    }

    // Setup buzzer if defined.
    if (BUZZER){
        // Set to make sure we know buzzer is enabled.
        BUZZ = true;

        // Pin declarations.
        pinMode(Buzzer, OUTPUT);   // Declaring buzzer as an output.
        digitalWrite(Buzzer, LOW); // Setting it to OFF.

        // Beep.
        digitalWrite(Buzzer, HIGH); // Activates the buzzer.
        delay(SHORT_PERIOD); // Delay 0.2 seconds.
        digitalWrite(Buzzer, LOW); // Deactivates the buzzer.
    } else {
        // Set to make sure we know LED is disabled.
        BUZZ = false;
    }

}

// Just a set of beeps.
int FlashBeep::beep(unsigned int PERIOD, int BEEPS) {
    // State variable.
    int STATES = 0;

    // Make sure buzzer is enabled.
    if(!BUZZ){
        // Retrun 2 if the buzzer is not enabled.
        return 2;
    }

    // Check to make sure beeps aint 0.
    if (BEEPS > 0) {
        STATES = BEEPS * 2;
    } else {
        // Return 1 if its 0 (user needs to handle this). 
        return 1;
    }

    startMillis = millis();

    for(int i = 0; i < STATES;) {
        if ((unsigned long)millis() - BuzzerpreviousMillis >= PERIOD) {
            // save the last time you buzzed the buzzer.
            BuzzerpreviousMillis = millis();

            // if the buzzer is off turn it on and vice-versa:
            if (BuzzerState == LOW) {
                BuzzerState = HIGH;
            } else {
                BuzzerState = LOW;
            }

            // Increment counter if 
            i++;

        }

        // Set the buzzer.
        digitalWrite(Buzzer, BuzzerState);
    }

    // Return 0 (good);
    return 0;
}

// Just a set of flashes.
int FlashBeep::flash(unsigned int PERIOD, int FLASHES, int LED) {
    // State variable.
    int STATES = 0;
    int LEDSTATE = LOW;

    // Make sure LED is enabled.
    if(!RGB){
        // Retrun 3 if the LED is not enabled.
        return 3;
    }

    // Check to make sure beeps aint 0.
    if (FLASHES > 0) {
        STATES = FLASHES * 2;
    } else {
        // Return 1 if its 0 (user needs to handle this). 
        return 1;
    }

    startMillis = millis();

    for(int i = 0; i < STATES;) {
        if ((unsigned long)millis() - RGBpreviousMillis >= PERIOD) {
            // save the last time you buzzed the buzzer.
            RGBpreviousMillis = millis();

            // if the LED is off turn it on and vice-versa:
            if (LEDSTATE == LOW) {
                LEDSTATE = HIGH;
            } else {
                LEDSTATE = LOW;
            }

            // Increment counter if 
            i++;

        }

        // Set the LED.
        digitalWrite(LED, LEDSTATE);
    }

    // Return 0 (good);
    return 0;
}

// Flash and beep (every state has both a flash and beep).
int FlashBeep::flashBeep(unsigned int PERIOD, int FLABEEPS, int LED) {
     // Method variables.
    int STATES = 0;
    int LEDSTATE = LOW;
    
    // Make sure buzzer is enabled.
    if(!BUZZ){
        // Retrun 2 if the buzzer is not enabled.
        return 2;
    }

    // Make sure LED is enabled.
    if(!RGB){
        // Retrun 3 if the LED is not enabled.
        return 3;
    }

    // Check to make sure beeps aint 0.
    if (FLABEEPS > 0) {
        STATES = FLABEEPS * 2;
    } else {
        // Return 1 if its 0 (user needs to handle this). 
        return 1;
    }

    startMillis = millis();

    for(int i = 0; i < STATES;) {
        if ((unsigned long)millis() - previousMillis >= PERIOD) {
            // save the last time you buzzed the buzzer.
            previousMillis = millis();

            // if the LED is off turn it on and vice-versa:
            if (LEDSTATE == LOW) {
                LEDSTATE = HIGH;
            } else {
                LEDSTATE = LOW;
            }

            // if the buzzer is off turn it on and vice-versa:
            if (BuzzerState == LOW) {
                BuzzerState = HIGH;
            } else {
                BuzzerState = LOW;
            }

            // Increment counter if 
            i++;

        }

        // Set the LED.
        digitalWrite(LED, LEDSTATE);
        digitalWrite(Buzzer, BuzzerState);
    }

    // Return 0 (good);
    return 0;
}

// Handler for class, this takes all the pain out of flashbeep.
void FlashBeep::output(unsigned int PERIOD, int OUTPUTS, int LED = 0) {
    if (BUZZ && RGB) {
        if (LED != 0) {
            flashBeep(PERIOD, OUTPUTS, LED);
        }
    }

    if (!BUZZ && RGB) {
        flash(PERIOD, OUTPUTS, LED);
    }

    if (BUZZ && !RGB) {
        beep(PERIOD, OUTPUTS);
    }
}