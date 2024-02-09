#include "time2.h"
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP_EEPROM.h>

const char *ssid = "SkyRabbit";
const char *password = "20090602";

const long utcOffsetInSeconds = 3600;

char daysOfTheWeek[7][12] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                             "Thursday", "Friday", "Saturday"};

#define MILLISINMIN 60000
// #define MILLISINMIN 6000

#define TICKINTERVAL MILLISINMIN
#define LOOPINTERVAL 1000
#define LOOKBACKRANGE                                                          \
    20000 // the range behind the current millis in which to look for events

#define MAX_WAVES 16 // max num of waves

unsigned long nextTickMillis = MILLISINMIN;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

ESP8266WebServer server(80);

struct wave waves[MAX_WAVES];

byte numWaves = 0;

struct time currentTime;

void fetchNewTime();
void tick();
void printTime(struct time *timep);
void updateLEDs();
void updateCurrentOrNextWaveIndex();

void handlePost() {
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
          const char* name = item["name"];

          JsonObject color = item["color"];
          byte color_r = color["r"];
          byte color_g = color["g"];
          byte color_b = color["b"];
          byte color_a = color["a"];

          byte startHour = item["startHour"];
          byte startMinute = item["startMinute"];
          byte endHour = item["endHour"];
          byte endMinute = item["endMinute"];
          byte inDuration = item["inDuration"];
          i++;
        }

        server.send(200, "text/plain", "POST data received and parsed");
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}
void handleGet() { server.send(200, "text/plain", "Hello BOI!"); }

void setup() {
    Serial.begin(9600);

    // set up wifi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

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
    nextTickMillis = secsToNextTick * 1000;

    // set up EEPROM
    EEPROM.begin(MAX_WAVES * sizeof(wave) + 1); // + 1 for storing numWaves (byte)
    EEPROM.put(0, numWaves);
    EEPROM.put(1, waves);
}

void loop() {
    fetchNewTime();
    tick();
    printTime(&currentTime);
    server.handleClient();
    delay(LOOPINTERVAL);

    // find next wave i
    // if it has started
    //   get brightness
    //   update LEDs
}

void tick() {
    unsigned long currentMillis = millis();
    if (currentMillis + TICKINTERVAL < currentMillis)
        // warped
        return;
    if (nextTickMillis > currentMillis)
        return;

    // update intreval
    nextTickMillis += TICKINTERVAL;
    // update time
    Serial.println("tick");
    goNextMin(&currentTime);
}

void fetchNewTime() {
    // fetch at midnight
    static struct time nextFetchTime = {.hour = 0, .minute = 0};

    if (timeDiff(&currentTime, &nextFetchTime) != 0)
        return;
    // update intreval
    // fetch every 12 hours
    nextFetchTime.hour = (nextFetchTime.hour + 12) % 24;

    // fetch and set new time
    Serial.println("fetch new time");
    timeClient.update();
    // currentTime.day = timeClient.getDay();
    currentTime.hour = timeClient.getHours();
    currentTime.minute = timeClient.getMinutes();
    nextTickMillis = millis() + MILLISINMIN;
}

void printTime(struct time *timep) {
    char s[20];
    formatTime(s, timep);
    Serial.println(s);
}

void updateLEDs() {
    static int nextWaveIndex = findNextWaveIndex(waves, numWaves, &currentTime);
    static struct time *nextWaveStartTimep = &waves[nextWaveIndex].startTime;
    bool currentlyInWave = false;
    bool isFullBright = false;

    if (!currentlyInWave) {
        if (timeDiff(&currentTime, nextWaveStartTimep) != 0)
            return;
        // reached next wave
        currentlyInWave = true;
        if (++nextWaveIndex == numWaves)
            nextWaveIndex = 0;
        nextWaveStartTimep = &waves[nextWaveIndex].startTime;
    }
    // update leds
    if (!isFullBright) {
        // update brightness
    }

    // TODO exit wave
}
