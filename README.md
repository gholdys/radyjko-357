# Radyjko 357 (raˈdɨjkɔ 357)

Firmware for "Radyjko 357" - a standalone, battery-driven device for playing probably the best independent radio station in Poland: [Radio 357](https://radio357.pl/)

## Hardware

The device is basically a internet radio player. It is not limited to only "Radio 357" but can easily play any Revma stream with a simple configuration change. The stream can be either MP3 or AAC. The device can connect via WiFi to a Revma host and parse the redirection instruction that Revma uses to direct the client to the actual audio stream.

I've built "Radyjko 357" around [Adafruit Feather M0 WiFi with ATWINC1500](https://www.adafruit.com/product/3010) and [Adafruit Music Maker FeatherWing](https://www.adafruit.com/product/3357). These are not the cheapest components for building an internet radio player, but this is what I had lying around and since they are part of the same system (Adafruit Feather), they are perfectly compatible. The loudspeaker was part of a two-piece set from Tracer called Orlando 2.0 (shown below). It's a USB speaker set with an amplifier and volume control.

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

---

## Software

The whole software (or even firmware) for "Radyjko 357" is in just one Arduino source file: `radyjko-357.ino`. 

### Dependencies

The program requires four libraries (available in Arduino IDE):
- SPI
- WiFi101
- SD
- Adafruit_VS1053

There's also a header file with firmware patches for VS1053b - the MP3 decoder. **Applying those patches is crucial as without them, the Adafruit Music Maker FeatherWing will not be able to play chunks of MP3 or AAC**. You will find the call that applies the patches in the `initAudioPlayer()` function. 

Another piece of firmware that needs updating, for this whole contraption to work, is the ATWINC1500 WiFi module. The Arduino IDE contains a special tool for doing just that. The update procedure is a bit involved, but fortunately the folks at Adafruit did a good job at describing this process on this [website](https://learn.adafruit.com/adafruit-feather-m0-wifi-atwinc1500/updating-firmware).


### Configuration

The configuration for "Radyjko 357" is stored on a microSD card. The card need to be in FAT32 format so that it can be read using the standard SD library for Arduino. Besides that, there are no special requirements. The amount of data stored on the card is quite small:
- one configuration text file
- few small MP3 files with jingles and warning sounds

The configuration file should be called `config.txt` and contain just three parameters:
- `wifi-ssid`: SSID for your WiFi access point 
- `wifi-password`: the password for the WiFi access point
- `stream-id`: Revma stream ID. For Radio 357 these are 'ye5kghkgcm0uv' for AAC format and 'an1ugyygzk8uv' for MP3

Thus, the file will look something like this:

    wifi-ssid=<my WiFi SSID>
    wifi-password=<my WiFi password>
    stream-id=ye5kghkgcm0uv

Besides the configuration file, the SD card will also need to store some MP3 files. At this point, only one is necessary: `jingle.mp3`. This file will be played by the player before it is ready to play the stream. It improves the perceived performance as the device does need a few seconds to connect to the WiFi access point then to Revma host and start playing the stream. Without the jingle, you would hear only silence for a few seconds and that could be a bit confusing. 


