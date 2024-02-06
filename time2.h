#ifndef TIME_H
#define TIME_H

struct time {
    int hour;
    int minute;
    // int day;
};

struct wave {
    struct time startTime;
    struct time endTime;
    int inDuration;
    int fullBrightness;
    // color
};

int timeDiff(struct time *a, struct time *b);
char *formatTime(char s[], struct time *t);
void goNextMin(struct time *t);
int findNextWaveIndex(struct wave waves[], int totalWaves,
                      struct time *currentTimep);

#endif
