#pragma once
#include "Arduino.h"

const int LONG_PERIOD = 1000; // Long period LED/Buzzer will flash (milliseconds).
const int MEDIUM_PERIOD = 500; // Medium period LED/Buzzer will flash (milliseconds).
const int SHORT_PERIOD = 200; // Short period LED/Buzzer will flash (milliseconds).

// LED related variables and const.
const int RGBgreen = A1; // Rgb green.
const int RGBblue = A2; // Rgb blue.
const int RGBred = A3; // Rgb Red.

class FlashBeep {
    public:
        // Time variables.
        unsigned long startMillis, previousMillis;

        // Buzzer related variables and const.
        bool BUZZ = false;
        const int Buzzer = 4; // Onboard buzzer.

        int BuzzerState = LOW; // State used to set the buzzer.

        unsigned long BuzzerpreviousMillis = 0; // will store last time buzzer was updated.

        // LED related variables and const.
        bool RGB = false;

        unsigned long RGBpreviousMillis = 0; // will store last time LED was updated.

        void begin(bool BUZZ, bool LED);
        void output(unsigned int PERIOD, int FLABEEPS, int LED);

    private:
        int beep(unsigned int PERIOD, int STATES);
        int flash(unsigned int PERIOD, int STATES, int LED);
        int flashBeep(unsigned int PERIOD, int STATES, int LED);
};