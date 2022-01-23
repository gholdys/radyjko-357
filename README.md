# Radyjko 357 (raˈdɨjkɔ 357)

Firmware for *Radyjko 357* - a standalone, battery-driven device for playing probably the best independent, internet radio station in Poland: [Radio 357](https://radio357.pl/)

[![Radyjko 357](/pics/yt.jpg)](https://youtu.be/wZRowB8RlqE "Radyjko 357")

## Hardware

The device is basically a WiFi radio player. It is actually not limited only to "Radio 357", but I've built it with this particular radio station in mind and tested it only on this radio station. The device can easily play any *Revma* stream with a simple configuration change. The stream can be either MP3 or AAC. The device can connect via WiFi to a *Revma* host and parse the redirection instruction that *Revma* uses to direct the client to the actual audio stream.

I've built *Radyjko 357* around [Adafruit Feather M0 WiFi with ATWINC1500](https://www.adafruit.com/product/3010) and [Adafruit Music Maker FeatherWing](https://www.adafruit.com/product/3357). These are not the cheapest components for building an internet radio player, but this is what I had lying around and since they are part of the same system (Adafruit Feather), they are perfectly compatible. The loudspeaker was part of a two-piece set from *Tracer* called *Orlando 2.0* (shown below). It's a USB speaker set with an amplifier and volume control.

![alt text](/pics/P1160268.webp)

Other components include a LiPo 2500mAh battery, a LiPo charging module, a potentiometer for volume control, some resistors and a [FeatherWing Doubler](https://www.adafruit.com/product/2890):

![alt text](/pics/P1160274.webp)

After connecting, the components looked like this:

![alt text](/pics/P1160287.webp)

Note the volume control thingy in the bottom, right corner of the photo above. It contains a simple amplifier that works quite well in this setup even though it was meant to work with 5V and gets only ~3.7V.

The housing of each speaker is mostly empty...

![alt text](/pics/P1160272.webp)

... so it can easily fit all components: 

![alt text](/pics/P1160289.webp)

![alt text](/pics/P1160290.webp)

![alt text](/pics/P1160293.webp)

## Connections

Since the *Feather M0* and the *Music Maker* are both "Feathers", they share the same pinout. I've used the *FeatherWing Doubler* to connect the two boards and thus I did not have to solder any connections between them.

![alt text](/pics/P1160275.webp)

All I needed to do is to define the pins that *M0* will use to control the *Music Maker*. There are also two pins used for reading the volume potentiometer and the battery voltage divider:

| M0 Pin    | Connected to                      |
|----------:|-----------------------------------|
| 20        | VS1053 reset pin                  |
| 6         | VS1053 chip select pin            |
| 10        | VS1053 data select pin            |
| 9         | VS1053 data request interrupt pin |
| 5         | SD card chip select pin           |
| A5        | Volume pot. input                 |
| A4        | Battery voltage input             |


---

## Software

The whole software (firmware) for *Radyjko 357* is in just one Arduino source file called `radyjko-357.ino`. 

### Dependencies

The program requires four libraries (available in Arduino IDE):
- SPI
- WiFi101
- SD
- Adafruit_VS1053

There's also a header file (`vs1053b-patches.h`) with firmware patches for *VS1053b* - the MP3 decoder. **Applying those patches is crucial as without them, the Music Maker will not be able to play chunks of MP3 or AAC audio stream**. You will find the call that applies the patches in the `initAudioPlayer()` function. 

Another piece of firmware that needs updating, for this whole contraption to work, is the *ATWINC1500* WiFi module. The Arduino IDE contains a special tool for doing just that. The update procedure is a bit involved, but fortunately the folks at Adafruit did a good job at describing this process on this [website](https://learn.adafruit.com/adafruit-feather-m0-wifi-atwinc1500/updating-firmware).


### Configuration

The configuration for *Radyjko 357* is stored on a microSD card. The card needs to be in FAT32 format so that it can be read using the standard SD library for Arduino. Besides that, there are no special requirements. The amount of data stored on the card is quite small:
- one configuration text file
- one small MP3 file with a jingle

The configuration file should be called `config.txt` and contain just three parameters:
- `wifi-ssid`: SSID for your WiFi access point 
- `wifi-password`: the password for the WiFi access point
- `stream-id`: *Revma* stream ID. For *Radio 357* these are 'ye5kghkgcm0uv' for AAC format and 'an1ugyygzk8uv' for MP3

Thus, the file will look something like this (for playing AAC stream):

    wifi-ssid=<your WiFi SSID>
    wifi-password=<your WiFi password>
    stream-id=ye5kghkgcm0uv

Besides the configuration file, the SD card will also need to store a MP3 file called `jingle.mp3`. This file will be played by the player before it is ready to play the stream. It improves the perceived performance as the device does need a few seconds to connect to the WiFi access point, then to *Revma* host, parse the redirect response from *Revma* main server and then start playing the audio stream. Without the jingle, you would hear only silence for a few seconds and that could be a bit confusing - especially since the device has no screen or even blinking LEDs. 

---

## Operation

*Radyjko 357* has only one control - the volume knob at the top. It's a potentiometer with a switch so it can be used to turn the device on and off and set the volume. 

After turning the device on, the firmware does the following sequence of operations:

1. Initializes the SD card reader
2. Reads the `config.txt` file
3. Initializes the *VS1053b* audio decoder chip
4. Starts playing the `jingle.mp3`
5. In the meantime, initializes the *ATWINC1500* WiFi module
6. Connects to `stream.rcs.revma.com` to obtain the URL for the audio stream
7. Waits for the jingle to stop playing and starts the main program loop

The main loop does two things:
1. Fetches the redirect instruction, parses it and opens the actual audio stream
2. Reads a chunk of data from the audio stream and into a ring buffer and then reads another chunk from that buffer and into the *VS1053b* decoder.

The ring buffer is currently only 256kB long so it does require a rather stable connection.

## Improvements
The biggest missing feature is a low battery voltage warning. There should be some audio notification, that warns the user when the battery voltage drops below a certain level. Currently, the device simply turns off when the battery voltage is low. This behavior is controlled by the LiPo charging module and protects the battery from being discharged below a safe level. 

Another useful feature, that I haven't implemented yet, is battery charging notification. There should be an audio notification, that would be played when the battery charging starts and another when the battery is fully charged.   
