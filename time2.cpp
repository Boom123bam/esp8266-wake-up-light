#include "time2.h"
#include <stdio.h>

int timeDiff(struct time *a, struct time *b) {
    // positive if a > b
    return (a->hour * 60 + a->minute) - (b->hour * 60 + b->minute);
}

char *formatTime(char s[], struct time *t) {
    snprintf(s, 100, "h: %d, m: %d", t->hour, t->minute);
    return s;
}

void goNextMin(struct time *t) {
    if (t->minute++ == 60) {
        t->minute = 0;
        t->hour = (t->hour + 1) % 24;
    }
}

int findNextWaveIndex(struct wave waves[], int totalWaves,
                      struct time *currentTimep) {
    int waveIndex = 0;
    struct time *waveEndTimep = &waves[0].endTime;
    // end of wave is before current time
    while (timeDiff(currentTimep, waveEndTimep) >= 0) {
        waveEndTimep = &waves[++waveIndex].endTime;
    }
    return waveIndex;
}

String formatWave(struct wave *wavePtr) {
    String waveInfo = "Start Time: " + String(wavePtr->startTime.hour) + ":" + String(wavePtr->startTime.minute) +
                      " - End Time: " + String(wavePtr->endTime.hour) + ":" + String(wavePtr->endTime.minute) +
                      ", Duration: " + String(wavePtr->inDuration) + " minutes, Color: (" +
                      String(wavePtr->red) + "," + String(wavePtr->green) + "," + String(wavePtr->blue) + ")";
    return waveInfo;
}

// int main() {
//     struct time currentTime = {.hour = 18, .minute = 00};

//     struct wave waves[] = {
//         {.startTime = {.hour = 15, .minute = 30},
//          .endTime = {.hour = 15, .minute = 50},
//          .inDuration = 20,
//          .fullBrightness = 100},

//         {.startTime = {.hour = 17, .minute = 30},
//          .endTime = {.hour = 17, .minute = 35},
//          .inDuration = 2,
//          .fullBrightness = 100},

//         {.startTime = {.hour = 19, .minute = 30},
//          .endTime = {.hour = 19, .minute = 35},
//          .inDuration = 2,
//          .fullBrightness = 100},

//         {.startTime = {.hour = 21, .minute = 30},
//          .endTime = {.hour = 21, .minute = 45},
//          .inDuration = 5,
//          .fullBrightness = 100},

//         {.startTime = {.hour = 23, .minute = 30},
//          .endTime = {.hour = 23, .minute = 45},
//          .inDuration = 5,
//          .fullBrightness = 100},

//     };

//     printf("n: %d\n", findNextWaveIndex(waves, sizeof(waves) / sizeof(waves[0]),
//                                         &currentTime));

//     return 0;
// }
