#include "time.h"
#include "types.h"
#include "user.h"

// implement some way to make this file usable by both user and kernel
// first remove dependency on `user.h` for `sleep(...)`
// add a busy-wait similar to `xchg`

/*
 * sleep (1 tick):
 *   acquire(&tickslock);
 *   currentticks = ticks;
 *   release(&tickslock);
 *   while (ticks == currentticks);
 */

uint millisToticks(uint millis) {
  return (millis * CLOCKTICKS_PER_SECOND) / 1000;
}

uint ticksTomillis(uint ticks) {
  return (ticks * 1000) / CLOCKTICKS_PER_SECOND;
}

int difftime(struct rtcdate a, struct rtcdate b) {
  return rtcToseconds(a) - rtcToseconds(b);
}

uint timespent(struct rtcdate start) {
  struct rtcdate current;
  cmostime(&current);
  return difftime(current, start);
}

void pause(uint millis) {
  // this will not work as expected
  // we are reading time from CMOS (accurate upto a second only)
  struct rtcdate start;
  cmostime(&start);
  uint seconds = millis / 1000;

  while (timespent(start) < seconds) {
    sleep(1);
  };
}

void pause2(uint millis) { sleep(millisToticks(millis)); }
