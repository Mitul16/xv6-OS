#ifndef TIME_H
#define TIME_H

#include "date.h"

// for `rtcdate` structure (no leap year, month 28-31...)
#define rtcToseconds(x)                                                        \
  (((((x.year * 12 + x.month) * 30 + x.day) * 24 + x.hour) * 60 + x.minute) *  \
       60 +                                                                    \
   x.second)

// see `lapic.c` for the LAPIC timer configuration
#define CLOCKTICKS_PER_SECOND 1000

extern void cmostime(struct rtcdate *);

uint ticksTomillis(uint);
uint millisToticks(uint);
void pause(uint);
void pause2(uint);
int difftime(struct rtcdate, struct rtcdate);
uint timespent(struct rtcdate);

#endif // TIME_H
