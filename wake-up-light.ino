#include "time2.h"
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
    String plain = server.arg("plain");
    Serial.println("Received POST data: " + plain);
    server.send(200, "text/plain", "POST data received: " + plain);
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}
void handleGet() {
  server.send(200, "text/plain", "getty");
}

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
