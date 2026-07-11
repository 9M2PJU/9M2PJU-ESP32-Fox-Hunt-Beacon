#pragma once

#include "display_config.h"

// Display abstraction layer. Compiles to no-op when no display is configured.
// Shows beacon status: callsign, fox ID, mode, state, timing, battery, AP IP.

// Initialize display hardware and show startup screen with callsign + version.
void displayInit(const char *callsign, const char *version);

// Show the startup splash screen: "CALLSIGN Fox" header + version below.
void displayStartupScreen(const char *callsign, const char *version);

// Show the main status screen.
void displayUpdate(const char *callsign, const char *foxId,
                   const char *modeStr, const char *stateStr,
                   const char *timingStr, const char *batteryStr,
                   const char *ipStr);

// Show the settings menu. selectedIndex is the highlighted item (0-based).
// itemCount is the total number of items. labels[] and values[] are the
// menu item labels and their current on/off or text values.
void displayMenu(int selectedIndex, int itemCount,
                 const char *const labels[], const char *const values[]);

// Show AP mode screen with SSID and IP.
void displayAPMode(const char *ssid, const char *ip);

// Turn display on or off (for eco mode).
void displayPower(bool on);

// Clear the display.
void displayClear();
