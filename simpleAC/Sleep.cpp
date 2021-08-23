#include "Sleep.h"

// This is the code that runs when the interrupt button is pressed and interrupts are enabled.
void wakeUpNow() {}

// Lets go to sleep.
void sleepNow(NFCReader &nfcReader, Data &data) {
    sleep_enable(); // Enables the sleep mode.
    attachInterrupt(0,wakeUpNow, triggerState);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Sleep mode is set here.

    #ifdef using_RC522
        nfcReader.PCD_SoftPowerDown(); // Set the power down bit to reduce power consumption.
    #endif

    #ifdef using_PN532
        nfcReader.shutDown(PN532_WAKEUP_SPI); // Set the power down bit to reduce power consumption.
    #endif

    sleep_mode(); // This is where the device is actually put to sleep.

    sleep_disable(); // First thing that is done is to disable the sleep mode.

    data.state = waitingOnCard;

    #ifdef using_RC522
        nfcReader.PCD_SoftPowerUp(); // Turn the reader back on.
    #endif

    detachInterrupt(0); // Disables the interrupt.

    data.startMillis = millis(); // Set the last start up time.
}