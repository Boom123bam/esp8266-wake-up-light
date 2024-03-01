#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP_EEPROM.h>
#include <TimeLib.h>
#include <Adafruit_NeoPixel.h>
#include "credentials.h"

// const long utcOffsetInSeconds = 3600;
const long utcOffsetInSeconds = 0;

#define MAX_WAVES 16       // max num of waves
#define LOOPINTERVAL 1000  // delay between loops

#define LED_PIN 3
#define LED_COUNT 5

#define PREVIEW_TIMEOUT 100

// set up LED strip
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

ESP8266WebServer server(80);


struct wave {
  byte startHour;
  byte startMinute;
  byte endHour;
  byte endMinute;
  byte inDuration;
  byte red;
  byte green;
  byte blue;
};

struct {
  byte r;
  byte g;
  byte b;
  byte brightness;
  byte nextWaveIndex;
  bool currentlyInWave;
} status;

struct {
  byte r;
  byte g;
  byte b;
  byte timeout;
} previewStatus;

struct wave waves[MAX_WAVES];

byte numWaves = 0;

void fetchNewTime();
void updateLEDs();
void updateCurrentOrNextWaveIndex();
String formatWave(struct wave *wavePtr);

void handleColorPreview() {
  Serial.println("Got post color request!");
  if (server.hasArg("plain")) {
    String input = server.arg("plain");
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, input);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }
    previewStatus.r = doc["r"];
    previewStatus.g = doc["g"];
    previewStatus.b = doc["b"];
    previewStatus.timeout = PREVIEW_TIMEOUT;

    server.send(200, "text/plain", "POST data received and parsed");
  } else {
  server.send(400, "text/plain", "Bad Request");
  }
}

void handlePostWaves() {
  Serial.println("Got post waves request!");
  if (server.hasArg("plain")) {
    // Get the JSON payload from the request
    String input = server.arg("plain");

    // String input;

    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, input);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    byte i = 0;
    for (JsonObject item : doc.as<JsonArray>()) {
      const char *name = item["name"];

      JsonObject color = item["color"];
      byte color_r = color["r"];
      byte color_g = color["g"];
      byte color_b = color["b"];

      byte startHour = item["startHour"];
      byte startMinute = item["startMinute"];
      byte endHour = item["endHour"];
      byte endMinute = item["endMinute"];
      byte inDuration = item["inDuration"];

      waves[i] = wave{
        .startHour = startHour,
        .startMinute = startMinute,
        .endHour = endHour,
        .endMinute = endMinute,
        .inDuration = inDuration,
        .red = color_r,
        .green = color_g,
        .blue = color_b,
      };

      i++;
    }
    numWaves = i;

    // save waves to EEPROM
    EEPROM.put(0, numWaves);
    EEPROM.put(1, waves);
    boolean ok = EEPROM.commit();
    Serial.println((ok) ? "EEPROM commit OK" : "EEPROM commit failed");

    Serial.println("\nRecieved waves:");
    printWaves();

    status.nextWaveIndex = findNextWaveIndex();
    status.currentlyInWave = false;

    server.send(200, "text/plain", "POST data received and parsed");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}
void handleGet() {
  server.send(200, "text/plain", "Hello BOI!");
  Serial.println("Got GET request");

  strip.setBrightness(127);
  // flash green
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, 100,255,100);
  }
  strip.show();
  delay(1000);
  showLEDs();
}

void setup() {
  Serial.begin(9600);

  // set up wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  // set up server
  server.on("/", HTTP_GET, handleGet);
  server.on("/", HTTP_POST, handlePostWaves);
  server.on("/color", HTTP_POST, handleColorPreview);
  server.begin();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // set up time
  timeClient.begin();
  fetchNewTime();
  int secsToNextTick = 60 - timeClient.getSeconds();


  // set up LED
  strip.begin();
  strip.clear();
  strip.show();

  // set up EEPROM
  EEPROM.begin(MAX_WAVES * sizeof(wave) + 1);  // + 1 for storing numWaves (byte)

  // get wave data from EEPROM
  // EEPROM.get(0, numWaves);
  // EEPROM.get(1, waves);

  // printWaves();
  // status.nextWaveIndex = findNextWaveIndex();
  // Serial.printf("waves: %d next: %d\n", numWaves, status.nextWaveIndex);
}

void loop() {
  fetchNewTime();
  server.handleClient();
  updateLEDs();
  delay(showPreview() ? 100 : LOOPINTERVAL);
  // Serial.println(String(hour()) + ":" + String(minute()) + ":" + String(second()));
}

void printWaves() {
  for (int j = 0; j < numWaves; j++) {
    Serial.print("Wave ");
    Serial.println(j + 1);
    Serial.println(formatWave(&waves[j]));
  }
}

String formatWave(struct wave *wavePtr) {
  String waveInfo = "Start Time: " + String(wavePtr->startHour) + ":"
                    + String(wavePtr->startMinute) + " - End Time: " + String(wavePtr->endHour) + ":" + String(wavePtr->endMinute) + ", Duration: " + String(wavePtr->inDuration) + "minutes, Color: (" + String(wavePtr->red) + "," + String(wavePtr->green) + "," + String(wavePtr->blue) + ")";
  return waveInfo;
}


void fetchNewTime() {

  // fetch at midnight
  static byte nextFetchHour;
  if (hour() != nextFetchHour) return;

  // update intreval
  // fetch every 12 hours
  nextFetchHour = 24 % (nextFetchHour + 12);

  // fetch and set new time
  Serial.println("fetch new time");

  timeClient.update();

  setTime(timeClient.getEpochTime());
}

void updateLEDs() {
  struct wave *nextWavep = &waves[status.nextWaveIndex];

  if (!status.currentlyInWave) {
    // Serial.printf("i: %d, min: %d\n", status.nextWaveIndex,nextWavep->startMinute);
    if (nextWavep->startHour != hour() || nextWavep->startMinute != minute())
      return;
    // reached next wave
    Serial.println("wave start");
    status.currentlyInWave = true;

    status.g = waves[status.nextWaveIndex].green;
    status.r = waves[status.nextWaveIndex].red;
    status.b = waves[status.nextWaveIndex].blue;
    status.brightness = 0;
    showLEDs();


    if (++status.nextWaveIndex == numWaves)
      status.nextWaveIndex = 0;
    nextWavep = &waves[status.nextWaveIndex];
  }

  byte currentWaveIndex = status.nextWaveIndex - 1 % numWaves;
  if (status.brightness != 255) {
    // update brightness
    int start = waves[currentWaveIndex].startHour * 60 + waves[currentWaveIndex].startMinute;
    int dur = waves[currentWaveIndex].inDuration;
    float current = hour() * 60 + minute() + second() / 60.0;
    status.brightness = 255 * (current - start) / dur;

    showLEDs();
  }

  // exit wave
  if (hour() > waves[currentWaveIndex].endHour || (hour() == waves[currentWaveIndex].endHour && minute() > waves[currentWaveIndex].endMinute)) {
    status.brightness = 0;
    showLEDs();
    status.currentlyInWave = false;
  }
}

void showLEDs() {
  strip.setBrightness(status.brightness);
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, status.r, status.g, status.b);
  }
  strip.show();  // Send the updated pixel colors to the hardware.
}

bool showPreview() {
  if (previewStatus.timeout == 0) {
    return false;
  }

  if (--previewStatus.timeout == 0) {
    showLEDs();
    return false;
  }

  strip.setBrightness(255);
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, previewStatus.r, previewStatus.g, previewStatus.b);
  }
  strip.show();
  return true;
}

int findNextWaveIndex() {
  int waveIndex = 0;
  struct wave *wavep = &waves[0];
  // end of wave is before current time
  while (timeCmp(wavep->endHour, wavep->endMinute) < 0 && waveIndex < numWaves) {
    wavep = &waves[++waveIndex];
  }
  return waveIndex % numWaves;
}

int timeCmp(byte h, byte m) {
  // compare now time with h:m
  // positive if h:m later in the same day
  int t = h * 60 + m;
  int now = hour() * 60 + minute();
  return t - now;
}
