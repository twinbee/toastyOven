# ToastyOven#
 is a repo for a very cheap Arduino-controller PCB flow / reflow oven for the after-population soldering of existing Printed circuit boards (PCBs).  Contributors are github user @twinbee @ILMPBx

The approach taken is low budget / minimum viable product (MVP). The oven features one LED for status, and one button for control. 

Many others have produced fancy ovens with OLED / TFT diplays and many buttons. This is not that.. This is minimal and cheap enough to get a good result; No more, no less.

Licensed under the Mozilla Public License 2.0 (MPL2). See LICENSE for more details. Copy right Matthew Bennett, Luke Andrews 2016.

## Bill of materials ##
1. A cheap toaster 
oven ($24 new or $15 thrifted)
2. Solid state relays capable of a few amps (two, about $9 each)
3. Arduino. I used an UNO I had lying around. ($25)
4. USB cable for programming or changing profile ($5)
5. Type K thermocouples (one or two, $11 each)
6. A way to get the thermocouple data to the arduino. We used a 1-wire board from 
7. Push button ($1)


Sum total is about $125


## Repo Structure ##
**duino** Contains the Arduino-based controller code. **designs** contains physical designs including some 3d-printable parts with sketchup models and wiring schematics. **processing**Contains code to take serial input from the arduino and plot it as a graph of temperature over time, for further tweaking
