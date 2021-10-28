/*
   Furnace Logger by DJ Sures (c)2021

   Updated: 2021/10/14

   1) Choose the wifi mode by uncomment AP_MODE or not
   2) Enter the SSID/PWD for either AP or Client mode
   3) Connect the Pilot voltage to A7 (pin 35)
   4) Connect the heat voltage (optional) to A6 (pin 34)
   5) Get USER Key and API Token from https://pushover.net/api
   6) Edit the #defines below
*/

// ----------------------------------------------------------------
// Network settings
// ----------------------------------------------------------------

// The WiFi mode for the ESP32 module
// Uncomment: Access Point Mode (configure AP mode settings below)
// Commented: Client Mode (configure client credentials below)
//#define AP_MODE

// AP Mode Settings
// This requires #define AP_MODE to be uncommented
// AP mode means you can connect to the ESP32 as if it's a router or server
// Default AP Mode IP Address of this device will be 192.168.50.1
// For an open system with no password, leave the password field empty
#define WIFI_AP_SSID "GetOffMyLawn"
#define WIFI_AP_PWD  ""

// Client Mode Credentials
// This requires #define AP_MODE to be commented
// Client mode means the ESP32 will connect to your router and you will have to know its IP Address
// Or, you can check the USB serial debug output
#define WIFI_CLIENT_SSID "myHomeSSID"
#define WIFI_CLIENT_PWD  "myPassword"

// PushOver credentials.
#define PUSH_OVER_API_TOKEN "akud4a9jasdfjs7hmt2a"
#define PUSH_OVER_USER_KEY "urn2q3sewrsagasdfkkg52d"

// Push Notification Title
#define PUSH_TITLE "Camp Fireplace"

// -----------------------------------------------------------------------------------------------------------------------------------------------------------

#include "heltec.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <WiFiAP.h>
#include <WebServer.h>
#include "Index.h"
#include <HTTPClient.h>
#include "ESP32Tone.h"
#include "sound-pilot-light-alarm.raw.h"
#include "sound-starting-up.raw.h"
#include "PushoverESP32.h"

WebServer      _server(80);

bool           _pilotStatus        = false;
int            _pilotVoltage       = 0;
byte           _pilotLightOffCount = 0;

bool           _heatStatus     = false;
int            _heatVoltage    = 0;

String         _scrollBuffer[6];
byte           _scrollPos      = 0;

String         _externalIP     = "n/a";

Pushover       _pushoverClient(PUSH_OVER_API_TOKEN, PUSH_OVER_USER_KEY);

unsigned long  _lastScreenUpdate   = 0;
long           _lastPushNotifyTime = 0;
long           _lastLocalAlarmTime = 0;
long           _lastHeartBeatAlarm = 0;

// -----------------------------------------------------------------------------------------------------------------------------------------------------------

void clearScroll() {

  _scrollPos = 0;

  for (byte i = 0; i < 5; i++)
    _scrollBuffer[i] = "";
}

void writeScroll(String s) {

  Heltec.display->clear();

  if (_scrollPos < 6) {

    _scrollBuffer[_scrollPos] = s;

    _scrollPos++;
  } else {

    for (byte i = 0; i < 5; i++)
      _scrollBuffer[i] = _scrollBuffer[i + 1];

    _scrollBuffer[5] = s;
  }

  for (byte i = 0; i < 6; i++)
    Heltec.display->drawString(0, i * 9, _scrollBuffer[i]);

  Heltec.display->display();

  Serial.println(s);
}

void getExternalIP() {

  HTTPClient http;

  http.begin("https://admin.synthiam.com/WS/WhereAmIV2.aspx");

  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {

    _externalIP = http.getString();
  } else {

    _externalIP = "Error";
  }

  http.end();
}

void Playsound_pilot_light_alarm() {

  for (int i = 0; i < 10; i++) {

    tone(26, 1186 + (i * 20), 75);

    delay(25);
  }

  for (long i = 0; i < sound_pilot_light_alarm_Size; i++) {

    dacWrite(26, sound_pilot_light_alarm[i]);

    delayMicroseconds(15);
  }

  dacWrite(26, 0);
}

void Playsound_starting_up() {

  for (long i = 0; i < sound_starting_up_Size; i++) {

    dacWrite(26, sound_starting_up[i]);

    delayMicroseconds(15);
  }

  dacWrite(26, 0);
}

void notify(String msg) {

  if (WiFi.status() != WL_CONNECTED)
    return;

  PushoverMessage myMessage;

  myMessage.title = PUSH_TITLE;
  myMessage.message = msg.c_str();

  _pushoverClient.send(myMessage);
}

void notifyAlarm(String msg) {

  if (WiFi.status() != WL_CONNECTED)
    return;

  PushoverMessage myMessage;

  myMessage.title = PUSH_TITLE;
  myMessage.message = msg.c_str();
  myMessage.sound = "spacealarm";

  _pushoverClient.send(myMessage);
}

String wl_status_to_string(wl_status_t status) {

  switch (status) {
    case WL_NO_SHIELD:
      return "No Wifi Shield";
    case WL_IDLE_STATUS:
      return "Idle";
    case WL_NO_SSID_AVAIL:
      return "No SSID Avail";
    case WL_SCAN_COMPLETED:
      return "Scan Completed";
    case WL_CONNECTED:
      return "Connected";
    case WL_CONNECT_FAILED:
      return "Connect Failed";
    case WL_CONNECTION_LOST:
      return "Connect Lost";
    case WL_DISCONNECTED:
      return "Disconnected";
    default:
      return "Unknown";
  }
}

String getTimeFormatted(int ms) {

  long secs = ms / 1000; // set the seconds remaining
  long mins = secs / 60; //convert seconds to minutes
  long hours = mins / 60; //convert minutes to hours
  long days = hours / 24; //convert hours to days

  secs = secs - (mins * 60); //subtract the coverted seconds to minutes in order to display 59 secs max
  mins = mins - (hours * 60); //subtract the coverted minutes to hours in order to display 59 minutes max
  hours = hours - (days * 24); //subtract the coverted hours to days in order to display 23 hours max

  return "Uptime: " + String(days) + " days " + String(hours) + ":" + String(mins) + ":" + String(secs);
}

void setup() {

  Playsound_starting_up();

  Heltec.begin(
    true,  // DisplayEnable Enable
    false, // LoRa Enable
    true); // Serial Enable

  Heltec.display->clear();

  Serial.begin(115200);

  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_max_tx_power(WIFI_POWER_19_5dBm);

#ifdef AP_MODE

  writeScroll("AP WiFi mode");

  writeScroll("SSID:" + WIFI_AP_SSID);
  writeScroll("PWD:" + WIFI_AP_PWD);

  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PWD);

  // Wait for WiFi to start
  delay(500);

  IPAddress Ip(192, 168, 50, 1);
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, Ip, NMask);

  IPAddress myIP = WiFi.softAPIP();
  writeScroll("IP: " + myIP);

  notify("AP Mode IP: " + myIP);

#else

  writeScroll("Client WiFi mode");

  WiFi.begin(WIFI_CLIENT_SSID, WIFI_CLIENT_PWD);
  writeScroll("SSID: " + String(WIFI_CLIENT_SSID));

  while (WiFi.status() != WL_CONNECTED)
    delay(1000);

  writeScroll("Connected!");
  writeScroll("IP: " + WiFi.localIP().toString());

  notify("Client mode IP: " + WiFi.localIP().toString());

#endif

  _server.on("/", handle_OnIndex);
  _server.onNotFound(handle_404);
  _server.begin();

  writeScroll("HTTP Server Started");

  writeScroll("Getting external IP");

  getExternalIP();

  writeScroll("Ext IP: " + _externalIP);

  notify("Got external IP: " + _externalIP);

  notify("Notifier started");
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------

// Reset function at address 0
void(* resetFunc) (void) = 0;

// -----------------------------------------------------------------------------------------------------------------------------------------------------------

void loop() {

  Heltec.display->clear();

  // disabled this because sometimes it would reboot because "wifi disconnected" status
  //#ifndef AP_MODE
  //  if (WiFi.status() != WL_CONNECTED) {
  //
  //    writeScroll("WiFi Disconnected");
  //    writeScroll("Rebooting...");
  //
  //    delay(2000);
  //
  //    resetFunc();
  //  }
  //#endif

  _pilotVoltage = analogRead(A7); // pin 35
  _heatVoltage = analogRead(A6); // pin 34

  if (_pilotVoltage > 100) {

    _pilotStatus = true;
    _pilotLightOffCount = 0;
  } else {

    if (_pilotLightOffCount == 100)
      _pilotStatus = false;

    _pilotLightOffCount++;
  }

  if (_heatVoltage > 100)
    _heatStatus = true;
  else
    _heatStatus = false;

  if (!_pilotStatus) {

    // limit speaker alarm to every 2 seconds
    if (millis() > _lastLocalAlarmTime + 2000) {

      Playsound_pilot_light_alarm();

      _lastLocalAlarmTime = millis();
    }

    // limit push alarm to every 5 minutes
    if (millis() > _lastPushNotifyTime + 300000) {

      notifyAlarm("Pilot light is out!");

      _lastPushNotifyTime = millis();
    }
  }

  // send heart beat every hour
  if (millis() > _lastHeartBeatAlarm + 3600000) {

    if (_pilotStatus)
      notify("Pilot: ON (" + String(_pilotVoltage * 0.0008056640625) + "v " + String(_pilotVoltage) + ")");
    else
      notify("Pilot: OFF (" + String(_pilotVoltage * 0.0008056640625) + "v " + String(_pilotVoltage) + ")");

    _lastHeartBeatAlarm = millis();
  }

  // force a reboot every 4 weeks
  if (millis() > 604800000) {

    writeScroll("4 weeks is up!");
    writeScroll("Rebooting...");

    delay(2000);

    resetFunc();
  }

  // update display every second
  if (millis() > _lastScreenUpdate + 1000) {

    _lastScreenUpdate = millis();

    Heltec.display->drawString(0, 0, "Local: http://" + WiFi.localIP().toString());
    Heltec.display->drawString(0, 9, "Ext: http://" + _externalIP);

    if (_heatStatus)
      Heltec.display->drawString(0, 18, "Heater: ON (" + String(_heatVoltage * 0.0008056640625) + "v " + String(_heatVoltage) + ")");
    else
      Heltec.display->drawString(0, 18, "Heater: OFF (" + String(_heatVoltage * 0.0008056640625) + "v " + String(_heatVoltage) + ")");

    if (_pilotStatus)
      Heltec.display->drawString(0, 27, "Pilot: ON (" + String(_pilotVoltage * 0.0008056640625) + "v " + String(_pilotVoltage) + ")");
    else
      Heltec.display->drawString(0, 27, "Pilot: OFF (" + String(_pilotVoltage * 0.0008056640625) + "v " + String(_pilotVoltage) + ")");

    Heltec.display->drawString(0, 36, "WiFi: " + wl_status_to_string(WiFi.status()));

    Heltec.display->drawString(0, 45, getTimeFormatted(millis()));

    Heltec.display->display();
  }

  _server.handleClient();
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// HTTP Handles
// -----------------------------------------------------------------------------------------------------------------------------------------------------------

void handle_OnIndex() {

  _server.send(200, "text/html", getIndexPage(_pilotStatus, _heatStatus, _pilotVoltage));
}

void handle_404() {

  _server.send(200, "text/html", "404 error");
}
