#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP_EEPROM.h>
#include <TimeLib.h>


const char *ssid = "SkyRabbit";
const char *password = "20090602";

// const long utcOffsetInSeconds = 3600;
const long utcOffsetInSeconds = 0;

#define MAX_WAVES 16       // max num of waves
#define LOOPINTERVAL 1000  // delay between loops

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
  wave *currentWave;
} ledStatus;


struct wave waves[MAX_WAVES];

byte numWaves = 0;

void fetchNewTime();
void updateLEDs();
void updateCurrentOrNextWaveIndex();
String formatWave(struct wave *wavePtr);


void handlePost() {
  Serial.println("Got post request!");
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

    server.send(200, "text/plain", "POST data received and parsed");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}
void handleGet() {
  server.send(200, "text/plain", "Hello BOI!");
}

void setup() {
  Serial.begin(9600);

  // set up wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n");

  // set up server
  server.on("/", HTTP_GET, handleGet);
  server.on("/", HTTP_POST, handlePost);
  server.begin();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // set up time
  timeClient.begin();
  fetchNewTime();
  int secsToNextTick = 60 - timeClient.getSeconds();

  // set up EEPROM
  EEPROM.begin(MAX_WAVES * sizeof(wave) + 1);  // + 1 for storing numWaves (byte)

  // get wave data from EEPROM
  EEPROM.get(0, numWaves);
  EEPROM.get(1, waves);

  printWaves();
}

void loop() {
  fetchNewTime();
  server.handleClient();
  delay(LOOPINTERVAL);
  Serial.println(String(hour()) + ":" + String(minute()) + ":" + String(second()));

  // find next wave i
  // if it has started
  //   get brightness
  //   update LEDs
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
  static int nextWaveIndex = findNextWaveIndex(waves, numWaves);
  static struct wave *nextWavep = &waves[nextWaveIndex];
  bool currentlyInWave = false;

  if (!currentlyInWave) {
    if (nextWavep->startHour != hour() || nextWavep->startMinute != minute())
      return;
    // reached next wave
    currentlyInWave = true;

    ledStatus.currentWave = &waves[nextWaveIndex];
    ledStatus.g = waves[nextWaveIndex].green;
    ledStatus.r = waves[nextWaveIndex].red;
    ledStatus.b = waves[nextWaveIndex].blue;
    ledStatus.brightness = 0;

    if (++nextWaveIndex == numWaves)
      nextWaveIndex = 0;
    nextWavep = &waves[nextWaveIndex];
  }
  if (ledStatus.brightness != 100) {
    // update brightness
    int start = ledStatus.currentWave->startHour * 60 + ledStatus.currentWave->startMinute;
    int dur = ledStatus.currentWave->inDuration;
    int current = hour() * 60 + minute();
    int brightness = 100 * (current - start) / dur;
  }
  // TODO update leds

  // exit wave
  if (hour() > ledStatus.currentWave->endHour || (hour() == ledStatus.currentWave->endHour && minute() > ledStatus.currentWave->endMinute )){
    currentlyInWave = false;
  }
}

int findNextWaveIndex(struct wave waves[], int totalWaves) {
  int waveIndex = 0;
  struct wave *wavep = &waves[0];
  // end of wave is before current time
  while (wavep->endHour < hour() || (wavep->endHour == hour() || wavep->endMinute < minute())) {
    wavep = &waves[++waveIndex];
  }
  return waveIndex;
}
