#ifndef TIME_H
#define TIME_H
#include "Arduino.h"

struct time {
    byte hour;
    byte minute;
};

struct wave {
    struct time startTime;
    struct time endTime;
    byte inDuration;
    byte red;
    byte green;
    byte blue;
};

int timeDiff(struct time *a, struct time *b);
char *formatTime(char s[], struct time *t);
void goNextMin(struct time *t);
int findNextWaveIndex(struct wave waves[], int totalWaves,
                      struct time *currentTimep);

#endif
