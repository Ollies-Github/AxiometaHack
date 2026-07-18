#pragma once
#include <stdint.h>
void displayBegin();
void displayLines(const char* const* lines, int count);
void displayMusicArt(const uint16_t* art, bool has);
void displayMusicStatic(const char* artist);
void displayMusicTitle(const char* title, int scroll);
void displayMusicProgress(int pct, bool playing, int vol);
void displayManeuver(const char* turn, long distanceM, const char* street);
