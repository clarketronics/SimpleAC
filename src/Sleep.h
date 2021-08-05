#pragma once
#include "Arduino.h"
#include "FlashBeep.h"
#include "Config.h"
#include "Helpers.h"
#include <avr/interrupt.h>
#include <avr/sleep.h>

//------Sleep functions------
void wakeUpNow(); // Wakup handler, no actual code here by default.
void sleepNow(NFCReader &nfcReader, Data &data); // Lets go to sleep.