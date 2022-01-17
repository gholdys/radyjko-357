# Radyjko 357 (raˈdɨjkɔ 357)
Firmware for "Radyjko 357" - a standalone, battery-driven device for playing probably the best independent radio station in Poland: [Radio 357](https://radio357.pl/)

## Hardware
The device is basically a internet radio player. It is not limited to only "Radio 357" but can easily play any Revma stream with a simple configuration change.

I've built "Radyjko 357" around [Adafruit Feather M0 WiFi with ATWINC1500](https://www.adafruit.com/product/3010) and [Adafruit Music Maker FeatherWing](https://www.adafruit.com/product/3357). These are not the cheapest components for building an internet radio player, but this is what I had lying around and since they are part of the same system (Feather), they are perfectly compatible. The loudspeaker was part of a two-piece set from Tracer called Orlando 2.0 (shown below). It's a USB speaker set with an amplifier and volume control.
![alt text](/pics/P1160268.jpg)

Other components include a LiPo 2500mAh battery, a LiPo charging module, a potentiometer for volume control, some resistors and a [FeatherWing Doubler](https://www.adafruit.com/product/2890):
![alt text](/pics/P1160274.jpg)

After connecting, the components looked like this:
![alt text](/pics/P1160287.jpg)

The housing of each speaker is mostly empty...
![alt text](/pics/P1160272.jpg)

... so it can easily fit all those components: 
![alt text](/pics/P1160289.jpg)
![alt text](/pics/P1160290.jpg)
![alt text](/pics/P1160293.jpg)

The result can be seen in action on this video (click to open in YouTube):
[![Radyjko 357](https://img.youtube.com/vi/wZRowB8RlqE/maxresdefault.jpg)](https://youtu.be/wZRowB8RlqE "Radyjko 357")

## Software
- ATWINC firmware update
- Patch VS1053
- config on the SD card (stream.rcs.revma.com)
-  


