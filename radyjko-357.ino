#include <SPI.h>
#include <WiFi101.h>
#include <Adafruit_VS1053.h>
#include <SD.h>
#include "vs1053b-patches.h"

const uint8_t VS1053_RESET_PIN = 20;
const uint8_t VS1053_CS_PIN = 6;
const uint8_t VS1053_DCS_PIN = 10;
const uint8_t VS1053_DREQ_PIN = 9;
const uint8_t CARD_CS_PIN = 5;
const uint8_t VOLUME_INPUT_PIN = A5;
const uint8_t BATTERY_VOLTAGE_INPUT_PIN = A4;

const uint8_t LOCATION_CHARS_TO_SKIP = strlen("Location: http://");
const char INITIAL_HOST[] = "stream.rcs.revma.com";  
const uint16_t CHAR_BUFFER_SIZE = 512;
const uint16_t AUDIO_READER_CHUNK_SIZE = 1024;
const uint8_t AUDIO_PLAYER_CHUNK_SIZE = 32;
const uint8_t AUDIO_PLAYER_CHUNK_BATCH = 32;
const uint32_t AUDIO_BUFFER_SIZE = 250000;
const uint32_t AUDIO_BUFFER_PLAY_THRESHOLD = 200000;

WiFiClient webClient;
String wifiSsid[2];
String wifiPass[2];
String streamId = "";

bool fetchingStreamUrl = false;
bool playingAudioStream = false;

char charBuffer[CHAR_BUFFER_SIZE];
uint32_t charBufferUsed = 0;
uint8_t audioReaderChunk[AUDIO_READER_CHUNK_SIZE]; 
uint32_t audioBufferReadIndex = 0;
uint32_t audioBufferWriteIndex = 0;
bool buffering = true;

Adafruit_VS1053_FilePlayer audioPlayer = Adafruit_VS1053_FilePlayer(VS1053_RESET_PIN, VS1053_CS_PIN, VS1053_DCS_PIN, VS1053_DREQ_PIN, CARD_CS_PIN);
uint8_t audioPlayerChunk[AUDIO_PLAYER_CHUNK_SIZE]; 

File bufferFile;

unsigned long readStartTime = 0;
unsigned long readTotalBytes = 0;
unsigned long playStartTime = 0;
unsigned long playTotalBytes = 0;

void setup() {
  pinMode(VOLUME_INPUT_PIN, INPUT_PULLUP);
  WiFi.setPins(8,7,4,2);
  Serial.begin(9600);
  
  // while (!Serial) {}

  initSdCard();
  readConfig();
  initAudioPlayer();
  checkBatteryLevel();
  initWiFi();
  fetchAudioStreamUrl();
  playJingle();  
}

void initSdCard() {
  if (!SD.begin(CARD_CS_PIN)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }

  // list files
  printDirectory(SD.open("/"), 0);
}

void readConfig() {
  File configFile = SD.open(F("config.txt"));

  if (!configFile) {
    Serial.print(F("Failed to find config file"));
    while(1);
  }

  const char* delimeter = "=";
  const uint8_t bufferLength = 200;
  char buffer[bufferLength];
  char* key;
  char* value;
  String line;
  int index = 0;

  Serial.println(F("--- Configuration ---")); 

  while (configFile.available()) {
    char c = configFile.read();
    if ( c == '\n' || index == bufferLength-1 ) {
      index = 0;
      
      line = String(buffer);
      line.trim();
      if ( line.length() > 0 && line.charAt(0) != '#' ) {
        key = strtok(buffer, delimeter);
        value = strtok(NULL, delimeter);
        Serial.print(key); Serial.print(" : "); Serial.println(value);
        setConfigParameter(key, value);
      }     
      // Clear the buffer 
      for ( int i=0; i<bufferLength; i++ ) {
        buffer[i] = 0;
      }
    } else {
      buffer[index++] = c;
    }    
  }
  configFile.close();
  Serial.println(); 
}

void setConfigParameter(char* key, char* value) {
  String keyString = String(key);
  String valueString = String(value);
  keyString.trim();
  valueString.trim();  

  if ( keyString.equals(F("wifi-ssid")) || keyString.equals(F("wifi-ssid-1")) ) {
    wifiSsid[0] = value;
  } else if ( keyString.equals(F("wifi-password")) || keyString.equals(F("wifi-password-1")) ) {
    wifiPass[0] = value;
  } else if ( keyString.equals(F("wifi-ssid-2")) ) {
    wifiSsid[1] = value;
  } else if ( keyString.equals(F("wifi-password-2")) ) {
    wifiPass[1] = value;
  } else if ( keyString.equals(F("stream-id")) ) {
    streamId = value;
  }
}

void initAudioPlayer() {
  if (!audioPlayer.begin()) {
    Serial.println(F("VS1053 not found"));
    while (1);  // don't do anything more
  }

  if (!audioPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT)) {
    Serial.println(F("VS1053_DREQ_PIN pin is not an interrupt pin"));
  }  

  audioPlayer.applyPatch(PATCHES, PATCHES_SIZE);
  
  while (!audioPlayer.readyForData()) {
    delay(100);
  }
  
  updateVolume();  
}

void initWiFi() {
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("WiFi shield not present"));
    // don't continue:
    while (true);
  }

  // attempt to connect to WiFi network:
  int status = WL_IDLE_STATUS;
  int accessPointIndex = 1;
  while (status != WL_CONNECTED) {
    accessPointIndex = 1-accessPointIndex;
    Serial.print(F("Attempting to connect to SSID: "));
    Serial.println(wifiSsid[accessPointIndex]);
    status = WiFi.begin(wifiSsid[accessPointIndex], wifiPass[accessPointIndex]);    
  }
  Serial.print(F("Connected to: "));
  printWiFiStatus();
}

void checkBatteryLevel() {
  float vBat = analogRead(BATTERY_VOLTAGE_INPUT_PIN);
  vBat *= 2;    // we divided by 2, so multiply back
  vBat *= 3.3;  // Multiply by 3.3V, our reference voltage
  vBat /= 1024; // convert to voltage
  Serial.print("VBat: " ); Serial.println(vBat);
  
  int count = 0;
  if ( vBat > 4.0 ) {
    count = 3;
  } else if ( vBat > 3.3 ) {
    count = 2;
  } else {
    count = 1;
  }

  for ( int i=0; i<count; i++) {
    playAlarm();
  }
}

void playJingle() {
  Serial.println(F("Playing jingle"));
  audioPlayer.startPlayingFile("/jingle.mp3");
}

void playAlarm() {
  audioPlayer.playFullFile("/alarm.mp3");
}

void waitForPlayer() {
  while(!audioPlayer.stopped()) {
    updateVolume();
    delay(100);
  }
}

void fetchAudioStreamUrl() {
  Serial.print(F("Fetching audio stream URL from "));
  Serial.print(INITIAL_HOST);
  Serial.print("/");
  Serial.println(streamId);
  get(INITIAL_HOST, ("/"+streamId).c_str());
  fetchingStreamUrl = true;
}

void openAudioStream(const char host[], const char path[]) {
  Serial.print(F("Started reading audio stream from: "));
  Serial.print(host);
  Serial.println(path);
  get(host, path);
  playingAudioStream = true;
}

void get( const char host[], const char path[] ) {
  if (webClient.connect(host, 80)) {
    Serial.println(F("Connected to server"));
    webClient.print(F("GET "));
    webClient.print(path);
    webClient.println(F(" HTTP/1.1"));
    webClient.print(F("Host: "));
    webClient.println(host);
    webClient.println(F("Connection: close"));
    webClient.println();
  }
}

void loop() {
  if ( fetchingStreamUrl ) {
    readAudioStreamUrl();
    return;
  }
  
  if ( playingAudioStream ) {
    readAudioChunk();
    playAudioChunk();
    return;
  }

  // do nothing forevermore:
  while (true);
}

void readAudioStreamUrl() {
  while (webClient.available() && charBufferUsed<CHAR_BUFFER_SIZE-1) {
    char c = webClient.read();
    charBuffer[charBufferUsed++] = c;    
  }

  if (!webClient.connected()) {
    fetchingStreamUrl = false;
    webClient.stop();
    charBuffer[charBufferUsed] = 0;
    Serial.println(F("\nRead:"));
    Serial.println(charBuffer);

    char host[25], path[60];
    findStreamUrl(charBuffer, host, path);
    openAudioStream(host, path);

    if ( bufferFile ) {
      Serial.println("Found an open buffer file. Closing...");
      bufferFile.close();      
    }

    bufferFile = SD.open("buffer", O_READ | O_WRITE);
    audioBufferReadIndex = 0;
    audioBufferWriteIndex = 0;
    // waitForPlayer();
  }
}

void readAudioChunk() {
  int len = webClient.read(audioReaderChunk, AUDIO_READER_CHUNK_SIZE);
  if ( len > 0 ) { 
    if ( canWriteToAudioBuffer() ) {
      writeToAudioBuffer(len);
    }
    readTotalBytes += len;
    if ( millis() - readStartTime > 1000 ) {
      Serial.print("Reading "); Serial.print(readTotalBytes); Serial.print(" bytes/second. Bytes in buffer: "); Serial.println( getAudioBytesInBuffer() );    
      readStartTime = millis();
      readTotalBytes = 0;
    }
  }
  
  if (!webClient.connected()) {
    playingAudioStream = false;
    webClient.stop();
    Serial.println();
    Serial.println(F("Connection closed. Reconnecting..."));
    playAlarm();
    fetchAudioStreamUrl();
  }
}

void playAudioChunk() {
  if ( buffering ) {
    if ( getAudioBytesInBuffer() < AUDIO_BUFFER_PLAY_THRESHOLD ) return;
    buffering = false;
  }

  updateVolume();
  for ( int i=0; i<AUDIO_PLAYER_CHUNK_BATCH; i++ ) {
    if ( audioPlayer.readyForData() && canReadFromAudioBuffer() ) {    
      readFromAudioBuffer();
      audioPlayer.playData(audioPlayerChunk, AUDIO_PLAYER_CHUNK_SIZE);
    } else if ( !canReadFromAudioBuffer() ) {
      Serial.println(F("Buffering..."));
      buffering = true;
      break;
    }
  }
}

uint32_t getAudioBytesInBuffer() {
  return ( audioBufferWriteIndex < audioBufferReadIndex ) ? getUnwrappedWriteIndex() - audioBufferReadIndex : audioBufferWriteIndex - audioBufferReadIndex;
}

bool canWriteToAudioBuffer() {
  if ( audioBufferWriteIndex == 0 && audioBufferReadIndex == 0 ) { // Special case for startup buffering
    return true;
  }
  return audioBufferWriteIndex+AUDIO_READER_CHUNK_SIZE < getUnwrappedReadIndex();
}

bool canReadFromAudioBuffer() {
  uint32_t unwrappedChunkEndIndex = audioBufferReadIndex+AUDIO_PLAYER_CHUNK_SIZE;
  return unwrappedChunkEndIndex < getUnwrappedWriteIndex();
}

uint32_t getUnwrappedWriteIndex() {
  return ( audioBufferWriteIndex < audioBufferReadIndex ) ? audioBufferWriteIndex+AUDIO_BUFFER_SIZE : audioBufferWriteIndex;
}

uint32_t getUnwrappedReadIndex() {
  return ( audioBufferWriteIndex <= audioBufferReadIndex ) ? audioBufferReadIndex : audioBufferReadIndex+AUDIO_BUFFER_SIZE;
}

void readFromAudioBuffer() {
  bufferFile.seek(audioBufferReadIndex);
  int r = bufferFile.read(audioPlayerChunk, AUDIO_PLAYER_CHUNK_SIZE);
  playTotalBytes += r;
  if ( millis() - playStartTime > 1000 ) {
    Serial.print("Playing "); Serial.print(playTotalBytes); Serial.print(" bytes/second. Bytes in buffer: "); Serial.println( getAudioBytesInBuffer() );    
    playStartTime = millis();
    playTotalBytes = 0;
  }
  audioBufferReadIndex = (audioBufferReadIndex+AUDIO_PLAYER_CHUNK_SIZE) % AUDIO_BUFFER_SIZE;
}

void writeToAudioBuffer( int chunkSize ) {
  uint32_t chunkEndIndex = audioBufferWriteIndex+chunkSize;
  if ( chunkEndIndex < AUDIO_BUFFER_SIZE ) {
    bufferFile.seek(audioBufferWriteIndex);
    bufferFile.write(audioReaderChunk, chunkSize);
    audioBufferWriteIndex = chunkEndIndex;
  } else {
    uint32_t partOneSize = AUDIO_BUFFER_SIZE-audioBufferWriteIndex;
    bufferFile.seek(audioBufferWriteIndex);
    bufferFile.write(audioReaderChunk, partOneSize);
    uint32_t partTwoSize = chunkEndIndex-AUDIO_BUFFER_SIZE;
    bufferFile.seek(0);
    bufferFile.write(audioReaderChunk+partOneSize, partTwoSize);
    audioBufferWriteIndex = chunkEndIndex-AUDIO_BUFFER_SIZE;
  }
}

void updateVolume() {
  uint16_t volumeControlValue = analogRead(VOLUME_INPUT_PIN);
  setVolume(volumeControlValue);
}

void findStreamUrl(char message[], char host[], char path[]) {
  String m = String(message);
  int lineStartIndex = m.indexOf('\n');
  int lineEndIndex = m.indexOf('\n', lineStartIndex+1);
  String locationLine = m.substring(lineStartIndex, lineEndIndex);  

  int pathStartIndex = locationLine.indexOf('/', LOCATION_CHARS_TO_SKIP+1);
  String hostStr = locationLine.substring(LOCATION_CHARS_TO_SKIP+1, pathStartIndex);
  String pathStr = locationLine.substring(pathStartIndex);
  strcpy(host, hostStr.c_str());  
  strcpy(path, pathStr.c_str());  
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print(F("IP Address: "));
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print(F("Signal strength (RSSI):"));
  Serial.print(rssi);
  Serial.println(F(" dBm"));
}

void applyPatches() {
  int i = 0;

  while (i<PATCHES_SIZE) {
      unsigned short addr, n, val;
      addr = PATCHES[i++];
      n = PATCHES[i++];
      if (n & 0x8000U) { /* RLE run, replicate n samples */
          n &= 0x7FFF;
          val = PATCHES[i++];
          while (n--) {
              audioPlayer.sciWrite(addr, val);
          }
      } else {           /* Copy run, copy n samples */
          while (n--) {
              val = PATCHES[i++];
              audioPlayer.sciWrite(addr, val);
          }
      }
  }

  audioPlayer.softReset();
}

void setVolume(uint16_t vol) {
    // Input value is 0..1023. 1023 is the loudest.
    uint8_t mappedValue = map(vol, 0, 1024, 100, 0); // 0..100%
    audioPlayer.setVolume(mappedValue,mappedValue);
}

void printDirectory(File dir, int numTabs) {
  Serial.println(F("--- SD card contents ---"));
  while(true) {
    
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i=0; i<numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs+1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
  Serial.println();
}