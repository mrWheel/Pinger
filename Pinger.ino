/*******************************************************************

 ESP32-Ping-With-Telegram

*/
#define _HOSTNAME     "PINGER"
#define _FW_VERSION   "v1.0.0 (13-04-2022)"
#define _USE_TELEGRAM
/*
   Arduino-IDE settings for ESP32 (Generic):

    - Board: "ESP32 Dev Module"
    - Upload Speed: "921600"
    - CPU Frequency: "80 MHz"
    - Flash mode: "QIO"
    - Flash size: "4M (32Mb)"
    - Partition Scheme: "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
    - Core Debug Level: "None"
    - PSRAM: "Disabled"
    - Arduino Runs On: "Core 1"
    - Events Runs On: "Core 0"
    - Port: "?"

**
** Formatting ( http://astyle.sourceforge.net/astyle.html#_Quick_Start )
**   - Allman style (-A1)
**   - tab 2 spaces (-s2)
**   - Indent 'switch' blocks (-S)
**   - Indent preprocessor blocks (-xW)
**   - Indent multi-line preprocessor definitions ending with a backslash (-w)
**   - Indent C++ comments beginning in column one (-Y)
**   - Insert space padding after commas (-xg)
**   - Attach a pointer or reference operator (-k3)
**
** use:  astyle <*.ino>
**
*/

/*
// ---------------------------------------------------------------
// Non Standard Libraries -
// ---------------------------------------------------------------
library ESP32Ping-master at version 1.7     in /libraries/ESP32Ping-master
library TelnetStream at version 1.2.2       in /libraries/TelnetStream
library CTBot at version 2.1.8              in /libraries/CTBot 
library ArduinoJson at version 6.19.3       in /libraries/ArduinoJson

// ---------------------------------------------------------------
// Standard Libraries - Already Installed if you have ESP32 set up
// ---------------------------------------------------------------
library WiFi at version 2.0.0               /esp32/2.0.2/libraries/WiFi
library WiFiClientSecure at version 2.0.0   /esp32/2.0.2/libraries/WiFiClientSecure
library ESPmDNS at version 2.0.0            /esp32/2.0.2/libraries/ESPmDNS
library FS at version 2.0.0                 /esp32/2.0.2/libraries/FS
library LittleFS at version 2.0.0           /esp32/2.0.2/libraries/LittleFS
// ---------------------------------------------------------------
*/

#include <WiFi.h>
#include "mySecrets.h"
#include <WiFiClientSecure.h>

#define PING_DEFAULT_TIMEOUT    2
#define PING_DEFAULT_COUNT      2
#define PING_DEFAULT_INTERVAL   2
#define PING_DEFAULT_SIZE      32
#define PING_MAX_DEVICES      254 // 254

#define MAX_STATE               2

#include <ESP32Ping.h>

#include <time.h>
#include "time_zones.h"

//-- next three lines before #include "Debug.h"
struct  tm timeInfo;
time_t  now;
char    thisTime[30] = {};

//#include <esp_wifi.h>
#include <soc/soc.h>
#include <soc/rtc_cntl_reg.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <LittleFS.h>

bool  gotNtpTime = false;
#include <TelnetStream.h>     // v1.2.2 - https://github.com/jandrassy/TelnetStream
#include "Debug.h"

//--------------------------------------------------------------------
// Additional Libraries - each one of these will need to be installed.
// -------------------------------------------------------------------

#include "CTBot.h"
CTBot myBot;

const char ssid[]     = WIFI_SSID;      //-- from mySecrets.h
const char password[] = WIFI_PASSWORD;  //-- from mySecrets.h

uint64_t thisChatId   = BOT_CHAT_ID;    //-- from mySecrets/h


#ifndef BUILTIN_LED
  #define BUILTIN_LED   2
#endif

// a variable to store telegram message data
TBMessage msg;

// Set your Static IP address
IPAddress local_IP(192, 168, 12, 184);
// Set your Gateway IP address
IPAddress gateway(192, 168, 12, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

struct _devData
{
  int8_t state;
  int8_t prevState;
  char   Descr[26];
} devData;

bool      WiFiStatus = false;

bool      reboot_request = false;

const char *ntpServer  = "pool.ntp.org";
const char *TzLocation = "Europe/Amsterdam";

uint32_t  getNtpTimeTimer = 0;

_devData  deviceInfo[260] = {};
char      cDescr[40]      = {};
char      cState[40]      = {};
int       dState          = 0;
char      cID[40]         = {};
int       ipUnderTest     = 2;
int       deviceStateInx  = 0;
char      cBuff[255]      = {};
char      cStartTime[50]  = {};
uint32_t  pingTimer       = 0;
uint32_t  pingKnownTimer  = 0;
uint32_t  scanStartTime   = 0;
bool      muteTelegram    = false;

//----------------------------------------------------------------------------
bool startWiFi()
{
  //uint32_t brown_reg_temp = READ_PERI_REG(RTC_CNTL_BROWN_OUT_REG);
  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  // Attempt to connect to Wifi network:
  DebugTf("Try to Connecting Wifi to SSID: [%s]\r\n", ssid);

  // Set WiFi to station mode and disconnect from an AP if it was Previously connected
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(_HOSTNAME);
  WiFi.begin(ssid, password);

  // Configures static IP address
  if (WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
  {
    DebugTf("local IP [%3d.%03d.%03d.%03d]\r\n", local_IP[0],         local_IP[1],     local_IP[2],     local_IP[3]);
    DebugTf("gateway  [%3d.%03d.%03d.%03d]\r\n", gateway[0],           gateway[1],      gateway[2],      gateway[3]);
    DebugTf("subnet   [%3d.%03d.%03d.%03d]\r\n", subnet[0],             subnet[1],       subnet[2],       subnet[3]);
    DebugTf("DNS(1)   [%3d.%03d.%03d.%03d]\r\n", primaryDNS[0],     primaryDNS[1],   primaryDNS[2],   primaryDNS[3]);
    DebugTf("DNS(2)   [%3d.%03d.%03d.%03d]\r\n", secondaryDNS[0], secondaryDNS[1], secondaryDNS[2], secondaryDNS[3]);
  }
  else
  {
    DebugTln("STA Failed to configure");
  }

  while (WiFi.status() != WL_CONNECTED)
  {
    Debug(".");
    delay(500);
  }
  Debugln(". connected!\r\n");
  DebugTf("Connected to SSID [%s]\r\n", WiFi.SSID());
  DebugT("Local IP  [");
  Debug(WiFi.localIP());
  Debugln("]");

  if (!MDNS.begin(_HOSTNAME))
  {
    DebugTln("Error setting up MDNS responder!");
    return false;
  }
  else
  {
    DebugTf("mDNS responder started '%s'\r\n", _HOSTNAME);
  }
  time(&now);

  Debugln("");
  DebugTln("WiFi connected");
  DebugT("IP address: ");
  Debugln(WiFi.localIP());

  return true;

} //  startWiFi()


//////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
void setup()
{
  //-- disable Brown Out detector
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  delay(100);
  Debugln("\nBooting ... \n");
  Debugf("[%s] %s  compiled [%s %s]\n", _HOSTNAME, _FW_VERSION, __DATE__, __TIME__);

  DebugTln("--------------------------------------------");
  DebugTf("ESP32-Ping-With-Telegram %s\n", _FW_VERSION);
  DebugTln("--------------------------------------------");

  pinMode(BUILTIN_LED, OUTPUT);
  for(int i=0; i<20; i++)
  {
    digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED)); //-- toggle
    delay(200);
  }

  digitalWrite(BUILTIN_LED, HIGH); //-- blue led on
  WiFiStatus = startWiFi();

  TelnetStream.begin();

  if (WiFiStatus)
  {
    configTime(0, 0, ntpServer);
    //-- Set environment variable with your time zone
    setenv("TZ", getTzByLocation(TzLocation).c_str(), 1);
    tzset();
    printLocalTime();
  }
  digitalWrite(BUILTIN_LED, LOW); //blue led off

  if (!LittleFS.begin())
  {
    DebugTln("Error init LittleFS ..");
  }

  writeDeviceId(20, "Willem's iMac (bedraad)", -MAX_STATE);
  writeDeviceId(97, "No Name", -MAX_STATE);
  readDevices();
  /***
    readDeviceId(20);
    deviceInfo[20].state = dState;
    DebugTf("20 state[%d], prevState[%d]\r\n", deviceInfo[20].state, deviceInfo[20].prevState);
    writeDeviceId(20, "iMac Willem", -MAX_STATE);
    readDeviceId(20);
    deviceInfo[20].state = dState;
    DebugTf("20 state[%d], prevState[%d]\r\n", deviceInfo[20].state, deviceInfo[20].prevState);
  ***/

#ifdef _USE_TELEGRAM

  // connect the ESP to the desired access point
  myBot.wifiConnect(ssid, password);

  // set the telegram bot token
  myBot.setTelegramToken(BOT_TOKEN);

  // check if all things are ok
  if (myBot.testConnection())
    DebugTln("testConnection OK\r\n");
  else  DebugTln("testConnection NOT OK\r\n");

  String stat = "Reboot\nDevice: " + String(_HOSTNAME) + "\nVer: " + _FW_VERSION + "\nRssi: " + String(WiFi.RSSI()) + "\nip: " +  WiFi.localIP().toString() + "\n/pinger";
  myBot.sendMessage(msg.sender.id, stat);
#endif

  scanStartTime   = millis() / 1000;
  ipUnderTest     = 2;
  pingTimer       = millis();
  pingKnownTimer  = millis();

  digitalWrite(BUILTIN_LED, HIGH);
  DebugTln("Now, go for it ..");
  DebugFlush();
  delay(2000);

} //  setup()


//----------------------------------------------------------------------------
void loop()
{
  if ((millis() - pingTimer) > 500)
  {
    pingTimer = millis();
    ipUnderTest++;
    if (ipUnderTest >= PING_MAX_DEVICES)
    {
      ipUnderTest=2;
      printLocalTime();
      //-- No no no!!!- readDevices();  //-- resets state & prevState!!
      DebugTf("Total time to scan all devices [%d] seconds\r\n\n"
              , (millis()/1000) - scanStartTime);
      scanStartTime = millis() / 1000;
    }
    pingDevice(ipUnderTest, PING_DEFAULT_COUNT);
  }

  //-- now test known UP device
  if ((millis() - pingKnownTimer) > 11000)
  {
    pingKnownTimer = millis();
    deviceStateInx = pingKnownDevices(deviceStateInx);
  }

  if (!gotNtpTime && (millis() - getNtpTimeTimer) > 20000)
  {
    getNtpTimeTimer = millis();
    printLocalTime();
  }

  if (reboot_request)
  {
    String stat = "Rebooting on request\nVer: " + String(_FW_VERSION) + "\nRssi: " + String(WiFi.RSSI()) + "\nip: " +  WiFi.localIP().toString() ;
    Debugln(stat);
    myBot.sendMessage(thisChatId, stat, "");
    delay(5000);
    ESP.restart();
    delay(1000);
  }

#ifdef _USE_TELEGRAM

  if (CTBotMessageText == myBot.getNewMessage(msg))
  {
    handleNewMessages(msg);
  }

#endif

} //  loop()

/* eof */
