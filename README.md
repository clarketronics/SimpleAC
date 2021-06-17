# SimpleAC
A product and idea by Chimpo McDoodle.

## Required libraries:
**[LinkedList](https://github.com/ivanseidel/LinkedList)**: This is used for managing card UID's.  
**[DFRobotDFPlayerMini](https://github.com/DFRobot/DFRobotDFPlayerMini)**: This is for the MP3 player module.  
**[MFRC522](https://github.com/miguelbalboa/rfid)**: This is for the RC522 reader module.  
**[PN532](https://github.com/adafruit/Adafruit-PN532)**:This is for the PN532 reader module.  
**SoftwareSerial**: This is required by the DFRobot library above and is part of the default install.  

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
| RBR | |
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