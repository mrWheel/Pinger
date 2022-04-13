//----------------------------------------------------------------------------
void handleNewMessages(TBMessage msg)
{
  DebugTln("handleNewMessages()");

  digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
#ifdef _USE_TELEGRAM

  DebugTf("Got the message [%s]\r\n", msg.text.c_str());

  String hi = "Pinger got: ";
  hi += msg.text;

  myBot.sendMessage(msg.sender.id, hi);

  if (msg.text == "/start")
  {
    DebugTf("start: @%04d-%02d-%02d %02d:%02d:%02d\r\n"
            , timeInfo.tm_year+1900, timeInfo.tm_mon, timeInfo.tm_mday
            , timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
    myBot.sendMessage(msg.sender.id, "try: /pinger");
    return;
  }

  else if ( (msg.text == "/pinger") || (msg.text == "/start") )
  {
    String reply = "Running ever since "+String(cStartTime)+"\n";
    reply += "Device: " + String(_HOSTNAME) + "\nVer: " + String(_FW_VERSION)
             + "\nRssi: " + String(WiFi.RSSI()) + "\n";
    reply += String(thisTime) + "\n";
    reply += "/set <id>  <Description>\n";
    reply += "/up: show Devices online\n";
    reply += "/down: show Known Devices down\n";
    reply += "/ping <id>\n";
    reply += "/mute: don't mention state change 'No Name' devices ";
    reply += "[" + String((muteTelegram?"On":"Off")) + "]\n";
    reply += "/unmute: mention state change 'No Name' devices\n";
    reply += "/reboot: reboot\n";
    reply += "/pinger: show this help\n";
    myBot.sendMessage(msg.sender.id, reply);
  }

  else if (msg.text == "/up")
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
    else                reply += "found " + String(onlineCount) + " devices";
    myBot.sendMessage(msg.sender.id, reply);
    return;
  }

  else if (msg.text == "/down")
  {
    DebugTln("/down: show Known offline Devices");
    int offLineCount = 0;
    String reply = "Devices offline:\n";
    for (int i=1; i<255; i++)
    {
      //-- if not definitely "Online" 
      if (deviceInfo[i].state < MAX_STATE)
      {
        IPAddress ip (192, 168, 12, i);
        if (  !(strncasecmp("No Name", deviceInfo[i].Descr, 7)==0) )
        {
          snprintf(cBuff, sizeof(cBuff), "[%2d/%2d] %3d.%03d.%03d.%03d - %s\n"
                   , deviceInfo[i].prevState, deviceInfo[i].state
                   , ip[0], ip[1], ip[2], ip[3], deviceInfo[i].Descr);
          reply += String(cBuff);
          offLineCount++;
        }
      }
    }
    if (offLineCount==0) reply += "no known devices offline!\n";
    else                 reply += "found " + String(offLineCount) + " devices";
    myBot.sendMessage(msg.sender.id, reply);
    return;
  }

  else if (strncasecmp("/set ", msg.text.c_str(), 5)==0 )
  {
    DebugTf("[%s]\r\n", msg.text.c_str());
    int servIpNum = 0;
    String commPart = msg.text.substring(5);
    commPart.trim();  // remove trailing and leading spaces
    int spaceIdx  = commPart.indexOf(" ");
    String servIp = commPart.substring(0, spaceIdx);
    servIp.trim();
    servIpNum = servIp.toInt();
    String servDescr = commPart.substring(spaceIdx+1);
    servDescr.trim();
    DebugTf("[%s/%03d] Changed Descr form [%s] ", servIp, servIpNum, deviceInfo[servIpNum].Descr);
    if (servDescr == servIp)
    {
      servDescr = "No Name";
    }
    servDescr += "                   "; // add some spaces to the end
    snprintf(deviceInfo[servIpNum].Descr, 26, "%-25.25s", servDescr.c_str());
    Debugf(" to [%s]\r\n", deviceInfo[servIpNum].Descr);

    if (readDeviceId(servIpNum) == servIpNum)
      writeDeviceId(servIpNum, deviceInfo[servIpNum].Descr, dState);
    else  writeDeviceId(servIpNum, deviceInfo[servIpNum].Descr, -1);

    String reply = "Description of " + String(servIpNum) + " is set to ["+servDescr+"]";
    myBot.sendMessage(msg.sender.id, reply);
    return;
  }
  else if (strncasecmp("/ping ", msg.text.c_str(), 6)==0 )
  {
    String servIp    = msg.text.substring(6);
    servIp.trim();
    int    servIpNum = servIp.toInt();
    String reply     = "Device 192.168.12."+String(servIpNum);
    if (readDeviceId(servIpNum) == servIpNum)
    {
      reply += " (" + String(cDescr) + ")";
    }
    reply += " is ";
    int8_t pReply = pingDevice(servIpNum, 5);
    if (pReply >= 1)        //-- not +MAX_STATE
      reply += "Online";
    else if (pReply <= -1)  //-- not -MAX_STATE
      reply += "Offline";
    else  reply += "Unknown["+String(pReply)+"]";
    DebugTln("Send Telegram:");
    Debugln(reply);
    //DebugTf("[%03d] before[%s]\r\n", servIpNum, deviceInfo[servIpNum].Descr);
    myBot.sendMessage(msg.sender.id, reply);
    //DebugTf("[%03d] >after[%s]\r\n", servIpNum, deviceInfo[servIpNum].Descr);
    return;
  }

  else if (msg.text == "/mute")
  {
    muteTelegram = true;
    myBot.sendMessage(msg.sender.id, "[mute=on] don't send state change 'No Name' devices");
    return;
  }

  else if (msg.text == "/unmute")
  {
    muteTelegram = false;
    myBot.sendMessage(msg.sender.id, "[mute=off] send state change all devices");
    return;
  }

#endif
}   //  handleNewMessages()
