#pragma once
#include <stdint.h>

// AX22 Port 3 LED button — a hardware push-to-talk for the voice AI.
void aiButtonBegin();
int  aiButtonPoll();   // +1 on press (edge), -1 on release (edge), 0 otherwise
void aiLedSet(bool on);
void aiLedTick();      // safety auto-off if the phone never sends "done"
