#include "SPIFFS.h"
#include <Arduino.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <chrono>
#include <ctime>
#include <time.h>

#include "../lib/functions/functions.hpp"
#include "../lib/network/network.hpp"
#include "../lib/schedule/schedule.hpp"
#include "../lib/server/include/server.hpp"
#include "../lib/shared_variable/shared_variable.hpp"
#include "../lib/subscription/subscription.hpp"
#include "../lib/switch/switch.hpp"
#include "../lib/test/test.hpp"
#include "../lib/timing/timing.hpp"
#include "FS.h"

bool blink = HIGH;

void listLittleFSFiles();

void listLittleFSFiles() {
  // File dir = LittleFS.open("/");
  // while (dir.next()) {
  //   Serial.print("File: ");
  //   Serial.println(dir.fileName());
  // }
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while (file) {
    Serial.print("File: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }
}

void setup() {
  Serial.begin(9600);
  preferences.begin("my-app", false);
  defaultSwitchState = preferences.getBool("defaultSwitchState", HIGH);
  establishNetwork(0);
  pinMode(SWITCH_PIN, OUTPUT);
  digitalWrite(SWITCH_PIN, defaultSwitchState);
  pinMode(LED_BUILTIN, OUTPUT);
  loadSchedulesFromEEPROM();
  startWebServer();
  if (SPIFFS.begin()) {
    Serial.println("LittleFS initilization successful");
    // listLittleFSFiles();
    Serial.println("-------end-of-dir-list---------");
  }
  if (!initializeTime()) {
    Serial.println("ALERT: TIME Initialization Failed !!!");
  }
}

void loop() {
  // if (isSavedSubscriptionActive()) {
  //   digitalWrite(SWITCH_PIN, switchStatus());
  // }
  digitalWrite(SWITCH_PIN, switchStatus());
  //------------------------time sync(1day)----------------------//
  unsigned long syncInterval = 24 * 60 * 60 * 1000; // 24 hours in
  static unsigned long lastSyncTime = 0;
  if ((millis() - lastSyncTime >= syncInterval) || syncronizeTime == true) {
    syncTime();
    lastSyncTime = millis();
    syncronizeTime = false;
  }
  //-------------------------------------------------------------------------//

  //------------------------wifi connection (1min)----------------------//
  unsigned long wifiConnectionTimeout = 60000;
  static unsigned long lastConnectionTime = 0;

  if (WiFi.status() != WL_CONNECTED &&
      millis() - lastConnectionTime >= wifiConnectionTimeout) {
    lastConnectionTime = millis();
    Serial.println("wifi disconnected attempting reconnection");
    establishNetwork(true);
  }
  //-----------------------------------------------------------------------//

  unsigned long serverRequestTimeout = 7200000; // 2 hrs
  static unsigned long lastServerRequestTime = 0;
  if (!isSavedSubscriptionActive() &&
      millis() - lastServerRequestTime >= serverRequestTimeout) {
    lastServerRequestTime = millis();
    subscriptionStatus();
  }

  //----------------------sync Server-----------------------//
  // if (syncronizeServer == true) {
  //   syncronizeServer = false;
  //   bool state = subscriptionStatus();
  //   if (isActive != state) {
  //     // handleMessage()
  //     ESP.restart();
  //   }
  // }
  //--------------------------------------------------------//

  //-----------------Wifi Setting Updated---------------//
  if (networkSettingChanged) {
    establishNetwork(0);
    networkSettingChanged = false;
  }

  // if (isSavedSubscriptionActive() != isActive) {
  //   Serial.printf("-----------saved : %d  , isActive : %d",
  //                 isSavedSubscriptionActive(), isActive);
  //   if (subscriptionStatus()) {
  //     Serial.println("changes detected starting server");
  //     ESP.restart();
  //   }
  // }
  displayTime();
  digitalWrite(LED_BUILTIN, blink);
  blink = !blink;
  delay(1000);
}
