#include "Unauthorised.h"

// If UID is incorrect do this.
void unauthorised(Data &data, FlashBeep &feedback) { 
  // Write the number of aunothorised scans to eeprom.
  int unAuthScan = EEPROM.read(unauthScanCountLocation);
  unAuthScan++;
  EEPROM.update(unauthScanCountLocation, unAuthScan);

  // Serial message to notify that UID is invalid.
  #ifdef debug
    Serial.println(F("Access not authorised."));
  #endif

  // Flash red led and beep a 3 times.
  feedback.output(SHORT_PERIOD, 3, RGBred);

  // Play the track assigned when a unauthorised card / implant is scanned.
  #ifdef using_MP3
    mp3Play(DFPlayer, unauthorisedTrack);
  #endif

  // Lockout all actions for set time.
  if (unAuthScan == 3) {
    digitalWrite(RGBred, HIGH);
    delay(data.lockout[0]);
    digitalWrite(RGBred, LOW);
  } else if (unAuthScan == 4) {
    digitalWrite(RGBred, HIGH);
    delay(data.lockout[1]);
    digitalWrite(RGBred, LOW);
  } else if (unAuthScan == 5) {
    digitalWrite(RGBred, HIGH);
    delay(data.lockout[2]);
    digitalWrite(RGBred, LOW);    
  } else if (unAuthScan >= 6) {
    digitalWrite(RGBred, HIGH);
    delay(data.lockout[3]);
    digitalWrite(RGBred, LOW);    
  }

  #ifdef Sleep
    data.startMillis = millis(); // Reset timeout counter.
  #endif
}