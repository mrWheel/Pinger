/*******************************************************************

 ESP32-Ping-With-Telegram

*/

#define _HOSTNAME     "PINGER" 
#define _FW_VERSION   "v1.0.0 (11-04-2022)"
#define _USE_TELEGRAM

#define _BOT_MTBS  5000  //-- mean time between scan messages
/*

~~~~~~~~~~~~

Using library WiFi at version 2.0.0 in folder: C:\ArduinoPortable\arduino-1.8.19\portable\packages\esp32\hardware\esp32\2.0.2\libraries\WiFi 
Using library WiFiClientSecure at version 2.0.0 in folder: C:\ArduinoPortable\arduino-1.8.19\portable\packages\esp32\hardware\esp32\2.0.2\libraries\WiFiClientSecure 
Using library ESPmDNS at version 2.0.0 in folder: C:\ArduinoPortable\arduino-1.8.19\portable\packages\esp32\hardware\esp32\2.0.2\libraries\ESPmDNS 
"C:\\ArduinoPortable\\arduino-1.8.19\\portable\\packages\\esp32\\tools\\xtensa-esp32-elf-gcc\\gcc8_4_0-esp-2021r2/bin/xtensa-esp32-elf-size" -A "C:\\Users\\James\\AppData\\Local\\Temp\\arduino_build_57156/ESP32-CAM-Video-Telegram_8.9.ino.elf"
Sketch uses 956193 bytes (30%) of program storage space. Maximum is 3145728 bytes.
Global variables use 63080 bytes (19%) of dynamic memory, leaving 264600 bytes for local variables. Maximum is 327680 bytes.
C:\ArduinoPortable\arduino-1.8.19\portable\packages\esp32\tools\esptool_py\3.1.0/esptool.exe --chip esp32 --port COM7 --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0xe000 C:\ArduinoPortable\arduino-1.8.19\portable\packages\esp32\hardware\esp32\2.0.2/tools/partitions/boot_app0.bin 0x1000 C:\Users\James\AppData\Local\Temp\arduino_build_57156/ESP32-CAM-Video-Telegram_8.9.ino.bootloader.bin 0x10000 C:\Users\James\AppData\Local\Temp\arduino_build_57156/ESP32-CAM-Video-Telegram_8.9.ino.bin 0x8000 C:\Users\James\AppData\Local\Temp\arduino_build_57156/ESP32-CAM-Video-Telegram_8.9.ino.partitions.bin 


*******************************************************************/

// ----------------------------
// Standard Libraries - Already Installed if you have ESP32 set up
// ----------------------------

#include <WiFi.h>
#include "mySecrets.h"
#include <WiFiClientSecure.h>

#define PING_DEFAULT_TIMEOUT  1
#define PING_DEFAULT_COUNT    2
#define PING_DEFAULT_INTERVAL 2
#define PING_DEFAULT_SIZE    32
#define PING_MAX_DEVICES    254 // 254

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

#include <UniversalTelegramBot.h>
//#include "UniversalTelegramBot.h"  // use local library which is a modified copy of an old version
// Library for interacting with the Telegram API
// Search for "Telegram" in the Library manager and install
// The universal Telegram library
// https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot

const char ssid[] = WIFI_SSID;    
const char password[] = WIFI_PASSWORD;  

#ifndef BUILTIN_LED
  #define BUILTIN_LED   2
#endif

// Set your Static IP address
IPAddress local_IP(192, 168, 12, 184);
// Set your Gateway IP address
IPAddress gateway(192, 168, 12, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

struct _devData {
  int8_t state;
  int8_t prevState;
  char   Descr[26];
} devData;

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

bool  WiFiStatus = false;

uint32_t  botTimer;     //-- last time messages' scan has been done
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



//----------------------------------------------------------------------------
void handleNewMessages(int numNewMessages, bool skip=false) 
{
  DebugTf("handleNewMessages %s", (skip ? "Skip":""));
  DebugTf("message #[%d]\r\n", numNewMessages);

  digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
  
  for (int i = 0; i < numNewMessages; i++) 
  {
    chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    DebugTf("Got a message %s\r\n", text.c_str());

    String from_name = bot.messages[i].from_name;
    if (from_name == "") from_name = "Guest";

    String hi = "Got: ";
    hi += text;
    
    if (!skip) bot.sendMessage(chat_id, hi, "Markdown");
    else break;
    
    client.setHandshakeTimeout(120000);

    if (text == "/pinger") 
    {
      //snprintf(thisTime, sizeof(thisTime), "%02d:%02d:%02d 20%02d-%02d-%02d"
      //                            , timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec
      //                            , (timeInfo.tm_year - 100), (timeInfo.tm_mon +1), timeInfo.tm_mday);

      String reply = "Running ever since "+String(cStartTime)+"\n";
      reply += "Device: " + String(_HOSTNAME) + "\nVer: " + String(_FW_VERSION)
                     + "\nRssi: " + String(WiFi.RSSI()) + "\n";
      reply += String(thisTime) + "\n";
      reply += "/set <*id*>  <*Description*>\n";
      reply += "/up: show Devices online\n";
      reply += "/down: show *Known* Devices down\n";
      reply += "/ping <*id*>\n";
      reply += "/reboot: reboot\n";
      reply += "/pinger: show this help\n";
      bot.sendMessage(chat_id, reply, "Markdown");
    }

    else if (text == "/up") 
    {
      DebugTln("/up: show online Devices");
      int onlineCount = 0;
      String reply = "Devices online:\n";
      for (int i=1; i<=PING_MAX_DEVICES; i++)
      {
        if (deviceInfo[i].state >= 2) 
        {
          IPAddress ip (192, 168, 12, i); 
          //if (readDeviceId(i) == i)
                snprintf(cBuff, sizeof(cBuff), " %3d.%03d.%03d.%03d - %s\n", ip[0], ip[1], ip[2], ip[3], deviceInfo[i].Descr);
          //else  snprintf(cBuff, sizeof(cBuff), " %3d.%03d.%03d.%03d\n", ip[0], ip[1], ip[2], ip[3]);
          reply += String(cBuff);
          onlineCount++;
        }
      }
      if (onlineCount==0) reply += "no online devices!\n";
      else                reply += "found *" + String(onlineCount) + "* devices";
      bot.sendMessage(chat_id, reply, "Markdown");
      return;
    }

    else if (text == "/down") 
    {
      DebugTln("/down: show *Known* offline Devices");
      int offLineCount = 0;
      String reply = "Devices offline:\n";
      for (int i=1; i<=255; i++)
      {
        if (deviceInfo[i].state <= -1) 
        {
          IPAddress ip (192, 168, 12, i); 
          //if (readDeviceId(i) == i)
          {
            if (  !(strncasecmp("No Name", deviceInfo[i].Descr, 7)==0) )
            {
              snprintf(cBuff, sizeof(cBuff), " %3d.%03d.%03d.%03d\n", ip[0], ip[1], ip[2], ip[3]);
              reply += String(cBuff);
              offLineCount++;
            }
          }
        }
      }
      if (offLineCount==0) reply += "no *known* devices offline!\n";
      else                 reply += "found *" + String(offLineCount) + "* devices";
      bot.sendMessage(chat_id, reply, "Markdown");
      return;
    }

    else if (strncasecmp("/set ", text.c_str(), 5)==0 )
    {
      DebugTf("[%s]\r\n", text.c_str());
      int servIpNum = 0;
      String commPart = text.substring(5);
      commPart.trim();  // remove trailing and leading spaces
      int spaceIdx  = commPart.indexOf(" ");
      String servIp = commPart.substring(0, spaceIdx);
      servIpNum = servIp.toInt();
      String servDescr = commPart.substring(spaceIdx+1);
      servDescr.trim();
      snprintf(deviceInfo[servIpNum].Descr, sizeof(deviceInfo[0].Descr), "%-25.25s", servDescr.c_str());

      if (readDeviceId(servIpNum) == servIpNum)
            writeDeviceId(servIpNum, deviceInfo[servIpNum].Descr, dState);
      else  writeDeviceId(servIpNum, deviceInfo[servIpNum].Descr, -1);

      String reply = "Description of *" + String(servIpNum) + "* is set to *"+servDescr+"*";
      bot.sendMessage(chat_id, reply, "Markdown");
      return;
    }
    else if (strncasecmp("/ping ", text.c_str(), 6)==0 )
    {
      String servIp    = text.substring(6);
      servIp.trim();
      int    servIpNum = servIp.toInt();
      String reply     = "Device 192.168.12."+String(servIpNum); 
      if (readDeviceId(servIpNum) == servIpNum)
      {
        reply += " (*" + String(cDescr) + "*)";
      }
      reply += " is "; 
      uint8_t pReply = pingDevice(servIpNum);
      if (pReply > 0)
            reply += "*On*line";
      else if (pReply < 0) 
            reply += "*Off*Online";
      else  reply = "*??*";
      bot.sendMessage(chat_id, reply, "Markdown");
      return;
    }

    else if (text == "/reboot") 
    {
      reboot_request = true;
      return;
    }
    
  } //-- handle messages

}   //  handleNewMessages()


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
  DebugTf("Local IP  [%s]\r\n", WiFi.localIP());

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

  writeDeviceId(20, "iMac Willem", -2);
  writeDeviceId(97, "No Name", -2);
  readDevices();
/***
  readDeviceId(20);
  deviceInfo[20].state = dState;
  DebugTf("20 state[%d], prevState[%d]\r\n", deviceInfo[20].state, deviceInfo[20].prevState);
  writeDeviceId(20, "iMac Willem", -2);
  readDeviceId(20);
  deviceInfo[20].state = dState;
  DebugTf("20 state[%d], prevState[%d]\r\n", deviceInfo[20].state, deviceInfo[20].prevState);
***/    
  // Make the bot wait for a new message for up to 60seconds
  //bot.longPoll = 60;
  bot.longPoll = 5;

  //client.setInsecure();
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); 
  
#ifdef _USE_TELEGRAM
  String stat = "Reboot\nDevice: " + String(_HOSTNAME) + "\nVer: " + _FW_VERSION + "\nRssi: " + String(WiFi.RSSI()) + "\nip: " +  WiFi.localIP().toString() + "\n/pinger";
  bot.sendMessage(chat_id, stat, "");

  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  DebugT("flush Telegram messages ..");
  while (numNewMessages) 
  {
    handleNewMessages(numNewMessages, true);
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    delay(1000);
  }
  Debugln(" done!");
#endif
  
  botTimer        = millis();
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
  if ((millis() - pingTimer) > 1000)
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
    pingDevice(ipUnderTest);
  }
  
  //-- now test known UP device
  if ((millis() - pingKnownTimer) > 10000)
  {
    pingKnownTimer = millis();
    deviceStateInx = pingKnownDevices(deviceStateInx);
  }

  if (!gotNtpTime && (millis() - getNtpTimeTimer) > 20000)
  {
    getNtpTimeTimer = millis();
    printLocalTime(); 
  }
 
#ifdef _USE_TELEGRAM
//-- workaround for esp32-arduino 2.02 bug
//-- https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot/issues/270#issuecomment-1003795884  client.setHandshakeTimeout(120000); 
  client.setHandshakeTimeout(120000); 
#endif

  if (reboot_request) 
  {
    String stat = "Rebooting on request\nVer: " + String(_FW_VERSION) + "\nRssi: " + String(WiFi.RSSI()) + "\nip: " +  WiFi.localIP().toString() ;
    Debugln(stat);
    bot.sendMessage(chat_id, stat, "");
    delay(5000);
    ESP.restart();
    delay(1000);
  }

#ifdef _USE_TELEGRAM
  if ((millis() - botTimer) > _BOT_MTBS )  
  {
    if (WiFi.status() != WL_CONNECTED) 
    {
      Debugln("***** WiFi reconnect *****");
      WiFi.reconnect();
      delay(5000);
      if (WiFi.status() != WL_CONNECTED) 
      {
        Debugln("***** WiFi rerestart *****");
        startWiFi();
      }
    }

    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) 
    {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    botTimer = millis();
  }
#endif

} //  loop()

/* eof */
