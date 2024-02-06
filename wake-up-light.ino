#include "time2.h"
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

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

unsigned long nextTickMillis = MILLISINMIN;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

ESP8266WebServer server(80);

struct wave waves[] = {
    {.startTime = {.hour = 15, .minute = 30},
     .endTime = {.hour = 15, .minute = 50},
     .inDuration = 20,
     .fullBrightness = 100},

    {.startTime = {.hour = 17, .minute = 30},
     .endTime = {.hour = 17, .minute = 35},
     .inDuration = 2,
     .fullBrightness = 100},

    {.startTime = {.hour = 19, .minute = 30},
     .endTime = {.hour = 19, .minute = 35},
     .inDuration = 2,
     .fullBrightness = 100},

    {.startTime = {.hour = 21, .minute = 30},
     .endTime = {.hour = 21, .minute = 45},
     .inDuration = 5,
     .fullBrightness = 100},

    {.startTime = {.hour = 23, .minute = 30},
     .endTime = {.hour = 23, .minute = 45},
     .inDuration = 5,
     .fullBrightness = 100},

};

int numWaves;

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

        for (JsonObject item : doc.as<JsonArray>()) {

            int startTime_hour = item["startTime"]["hour"];     // 15, 17, 19
            int startTime_minute = item["startTime"]["minute"]; // 30, 30, 30

            int endTime_hour = item["endTime"]["hour"];     // 15, 17, 19
            int endTime_minute = item["endTime"]["minute"]; // 50, 35, 35

            int inDuration = item["inDuration"];         // 20, 2, 2
            int fullBrightness = item["fullBrightness"]; // 100, 100, 100
            Serial.printf("sh: %d, sm: %d, eh: %d, em: %d, d: %d, b: %d\n",
                          startTime_hour, startTime_minute, endTime_hour,
                          endTime_minute, inDuration, fullBrightness);
        }

        server.send(200, "text/plain", "POST data received and parsed");
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}
void handleGet() { server.send(200, "text/plain", "getty"); }

void setup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    server.on("/", HTTP_GET, handleGet);
    server.on("/", HTTP_POST, handlePost);
    server.begin();

    timeClient.begin();
    fetchNewTime();
    int secsToNextTick = 60 - timeClient.getSeconds();
    nextTickMillis = secsToNextTick * 1000;

    numWaves = sizeof(waves) / sizeof(waves[0]);

    Serial.begin(9600);

    Serial.print("Open http://");
    Serial.print(WiFi.localIP());
    Serial.println("/ in your browser to see it working");
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
