/*
** This file should not be pushed to github!
** 
**/
//-- use: git update-index --assume-unchanged mySecrets.h
//-- and: use .gitignore with a line "mySectrets.h"


#ifndef MY_SECRETS_H
#define MY_SECRETS_H

#define WIFI_SSID     "MY-WIFI-SSID"
#define WIFI_PASSWORD "MY-WIFI-PASSWORD"

// you can enter your home chat_id, so the device can send you a reboot message, otherwise it responds to the chat_id talking to telegram

// see here for information about getting free telegram credentials
// https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
// https://randomnerdtutorials.com/telegram-esp32-motion-detection-arduino/

String chat_id = "MY-CHAR-ID";
#define BOTtoken "MY-BOT-TOKEN"  // your Bot Token (Get from Botfather)

#endif

/*eof*/
