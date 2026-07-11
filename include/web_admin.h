#pragma once

#include <Arduino.h>

// Web admin UI. Starts a WiFi AP with captive portal so a phone or laptop can
// connect and configure the beacon from a browser. Works on all ESP32 boards
// (all have WiFi). No external library needed beyond the Arduino ESP32 core.

// Starts the AP, DNS captive portal, and HTTP web server.
// apNamePrefix is used as the SSID prefix; the last 4 hex of MAC is appended.
void webAdminInit(const char *apNamePrefix);

// Stop the AP and web server (turns off WiFi radio).
void webAdminStop();

// Must be called from the main loop to handle web clients.
void webAdminLoop();

// Returns the AP IP address string (e.g. "192.168.4.1").
String webAdminGetIp();

// Returns the AP SSID string.
String webAdminGetSsid();

// Returns true if the web admin is running.
bool webAdminIsRunning();
