#pragma once
#include "Arduino.h"
#include "FlashBeep.h"
#include "Data.h"
#include "Config.h"
#include "Helpers.h"
#include "Sleep.h"

void learn(Data &data, NFCReader &nfcReader, FlashBeep &feedback, Helpers &helpers);
