#pragma once
void displayBegin();
void displayLines(const char* const* lines, int count);
void displayMusic(const char* title, const char* artist, int pct, bool playing, int vol);
void displayManeuver(const char* turn, long distanceM, const char* street);
