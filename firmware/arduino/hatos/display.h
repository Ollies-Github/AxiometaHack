#pragma once
#include <stdint.h>
void displayBegin();
void displayLines(const char* const* lines, int count);

// Theme: false = dark (default), true = off-white light. Takes effect on the
// next render, so callers should redraw the active screen after changing it.
void displaySetTheme(bool light);
bool displayThemeLight();

// Branded startup splash (hat emblem + wordmark + tagline).
void displayBootSplash();

// Home launcher: a one-app-at-a-time carousel. `id`/`name` are the focused app,
// `index`/`total` drive the dot pager, `wifi` lights the header signal glyph.
void displayLauncher(const char* id, const char* name, int index, int total, bool wifi);

// Polished counter screen (big centered value).
void displayCounter(int value);
void displayMusicArt(const uint16_t* art, bool has);
void displayMusicStatic(const char* artist);
void displayMusicTitle(const char* title, int scroll);
void displayMusicProgress(int pct, bool playing, int vol);
void displayManeuver(const char* turn, long distanceM, const char* street);

// Teleprompter: word-wrap `text` and draw a window of lines starting at
// `topLine` (the top/current line is highlighted). Returns the total number of
// wrapped lines so the caller can clamp scrolling. `teleprompterCountLines`
// returns that total without drawing.
int displayTeleprompter(const char* text, int topLine);
int teleprompterCountLines(const char* text);
