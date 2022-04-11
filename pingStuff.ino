

//----------------------------------------------------------------------------
int pingDevice(int devId)
{
  bool replyBool = false;
  IPAddress ip (192, 168, 12, devId); // The remote ip to ping
  char ipBuff[20] = {};
  
  if (devId == 184) return true;

  digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));

  snprintf(ipBuff, sizeof(ipBuff), "%03d.%03d.%03d.%03d", ip[0], ip[1], ip[2], devId);
  //DebugTf("ping ip[%s]\r\n", ipBuff);
  
  if (Ping.ping(ip))
  {
    deviceInfo[devId].state++;
    if (deviceInfo[devId].state >  2) deviceInfo[devId].state = 2;
  }
  else 
  {
    deviceInfo[devId].state--;
    if (deviceInfo[devId].state < -2) deviceInfo[devId].state = -2;
  }

  if (deviceInfo[devId].state > 1 && deviceInfo[devId].prevState > 1)
      DebugTf("[%03d][%2d/%2d] [%s] is UP\r\n", devId
                                  , deviceInfo[devId].prevState
                                  , deviceInfo[devId].state
                                  , ipBuff);
  else if (deviceInfo[devId].state < -1 && deviceInfo[devId].prevState < -1)
  { 
    if (!strncasecmp(deviceInfo[devId].Descr, ipBuff, 9)==0)
      DebugTf("[%03d][%2d/%2d] [%s] is DOWN\r\n", devId
                                  , deviceInfo[devId].prevState
                                  , deviceInfo[devId].state
                                  , ipBuff);
  }
  else  
  {
    if (deviceInfo[devId].prevState > deviceInfo[devId].state)
        DebugTf("[%03d][%2d/%2d] [%s] UP->DOWN\r\n", devId
                                                    , deviceInfo[devId].prevState
                                                    , deviceInfo[devId].state
                                                    , deviceInfo[devId].Descr);
    else if (deviceInfo[devId].prevState < deviceInfo[devId].state)
        DebugTf("[%03d][%2d/%2d] [%s] DOWN->UP\r\n", devId
                                                    , deviceInfo[devId].prevState
                                                    , deviceInfo[devId].state
                                                    , deviceInfo[devId].Descr);
    else 
    {
      DebugTf("[%03d][%2d/%2d] [%s] state unknown\r\n", devId
                                                    , deviceInfo[devId].prevState
                                                    , deviceInfo[devId].state
                                                    , deviceInfo[devId].Descr);
    }

    //-- write record if state has changed
    if (deviceInfo[devId].prevState != deviceInfo[devId].state)
    {
      if (deviceInfo[devId].state <= -2 || deviceInfo[devId].state >= 2)
      {
        DebugTf("[%03d][%2d/%2d] [%s] state changed!\r\n", devId
                                          , deviceInfo[devId].prevState
                                          , deviceInfo[devId].state
                                          , ipBuff);
#ifdef _USE_TELEGRAM
        String stat = String(ipBuff) +" ";
        stat += String(deviceInfo[devId].Descr) +" state changed to ";
        if (deviceInfo[devId].state <= -2)     stat += "*Off*line";
        else if (deviceInfo[devId].state >= 2) stat += "*Off*line";
        else                               stat += "*On*line";
        if ((deviceInfo[devId].state <= -2) || (deviceInfo[devId].state >= 2))
        {
          bot.sendMessage(chat_id, stat, "Markdown");
        }
#endif
        if (readDeviceId(devId) == devId)
        {    
          if ( !(strncasecmp("No Name", cDescr, 7)==0) )
                writeDeviceId(devId, cDescr, deviceInfo[devId].state);
          else  writeDeviceId(devId, "No Name", deviceInfo[devId].state);
        }
        else  writeDeviceId(devId, "No Name", deviceInfo[devId].state);
      }
    }

  }

  deviceInfo[devId].prevState = deviceInfo[devId].state;
  
  return deviceInfo[devId].state;
  
} //  pingDevice()


//----------------------------------------------------------------------------
int pingKnownDevices(int upInx)
{
  int pState, i = upInx;
  
  //if (deviceInfo[i].state >= 1)
  if (!strncasecmp("No Name", deviceInfo[i].Descr, 7)==0)
  {
    DebugTf("[%03d]        [%s] Known Device\r\n", i, deviceInfo[i].Descr);
    pState = pingDevice(i);
    
    if (pState <= 0)  
          DebugTf("[%03d]        [%s] is Offline\r\n", i, deviceInfo[i].Descr);
    else if (pState < 2)            
          DebugTf("[%03d]        [%s] state is unknown\r\n", i, deviceInfo[i].Descr);
  }
  i++;

  //-- skip to next Up device
  while((i++ < 255) && (strncasecmp("No Name", deviceInfo[i].Descr, 7)==0))
  {
    yield();
  }
  if (i >= 255) i = 1;
  //DebugTf("[%03d] next 'Known Device' [%s]\r\n", i, deviceInfo[i].Descr);
  return i;
  
} //  pingKnownDevice()
