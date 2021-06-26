# SimpleAC
A product and idea by Chimpo McDoodle.

## Required libraries:
**[LinkedList](https://github.com/ivanseidel/LinkedList)**: This is used for managing card UID's.
**[DFRobotDFPlayerMini](https://github.com/DFRobot/DFRobotDFPlayerMini)**: This is for the MP3 player module.
**[MFRC522](https://github.com/miguelbalboa/rfid)**: This is for the RC522 reader module.
**[PN532](https://github.com/clarketronics/PN532)**:This is for the PN532 reader module (includes power saving).
**SoftwareSerial**: This is required by the DFRobot library above and is part of the default install.

## Definition options:
The code below contains all the options on how you can set up your simpleAC. Removing the // befor a line to let the code know your using that part.

```c++
#define debug // Unmark this to enable output printing to the serial monitor.
//#define usingMP3 // Unmark this if using the df mp3player.
#define usingRC522 // Unmark this if you are using the RC522 13.56MHz NFC-HF RFID reader.
//#define usingPN532 // Unmark this if you are using the RC522 13.56MHz NFC-HF RFID reader.
//#define hardcodeUID // Unmark this if you want to hard code your UID's.
#define usingLED // Unmark this if you want to enable the RGB LED.
#define usingBuzzer // Unmark this if you want to enable the Buzzer.
#define usingRelays // Unmark this if you want to enable the Relays.
```

## Flash codes:
Within the SimpleAC there are several flash / beep codes, some are specific to start up and others are shown during operation that relay information to the user.

### Startup (fault codes):
**Note:** These codes are always the red LED each flash is accompanied by a beep (if enabled).
| LED flash count | Description |
| :---: | :---: |
| 2 | MP3 module failed |
| 3 | Reader module failed |

### Runtime:
**Note:** Each LED flash will be accompanied by a beep (if enabled).
| LED Sequence | Description |
| :---: | :---: |
| RRR | Not authorised |
| RRB | No access cards defined |
| RRG | No master defined, scan new master now |
| RBR | Card to remove was not found |
| RBB | |
| RBG | |
| RGR | Master scanned (clear mode), starts 10 second countdown |
| RGB | |
| RGG | |
| BRR | |
| BRB | |
| BRG | |
| BBR | Learning mode, Card removed |
| BBB | Clear mode |
| BBG | Learning mode, Card added |
| BGR | |
| BGB | |
| BGG | |
| GRR | |
| GRB | |
| GRG | |
| GBR | Boot up sequence |
| GBB | |
| GBG | |
| GGR | |
| GGB | Learning mode (Master scanned) entered or exited |
| GGG | Authorised, Authorised shut down |