#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "Config.h"
#include "FlashBeep.h"
#include "Helpers.h"
#include "Data.h"
#include "MP3.h"

enum inputState{
    Locked,
    Unlocked
};

void authorised(Data &data, FlashBeep &feedback);