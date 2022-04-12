

//----------------------------------------------------------------------------
int pingDevice(int devId, int pingCount)
{
  bool replyBool = false;
  IPAddress ip (192, 168, 12, devId); // The remote ip to ping
  char ipBuff[30] = {};
  
  if (devId == 184) return true;

  digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));

  if ( (strncasecmp("No Name", deviceInfo[devId].Descr, 7)==0) )
        snprintf(ipBuff, sizeof(ipBuff), "%03d.%03d.%03d.%03d", ip[0], ip[1], ip[2], devId);
  else  snprintf(ipBuff, sizeof(ipBuff), "%s", deviceInfo[devId].Descr);
  //DebugTf("ping ip[%s]\r\n", ipBuff);
  
  if (Ping.ping(ip, pingCount))
  {
    deviceInfo[devId].state++;
    if (deviceInfo[devId].state >  2) deviceInfo[devId].state = 2;
  }
  else 
  {
    deviceInfo[devId].state--;
    if (deviceInfo[devId].state < -2) deviceInfo[devId].state = -2;
  }

  if (deviceInfo[devId].state >= 2 && deviceInfo[devId].prevState >= 2)
      DebugTf("[%03d][%2d/%2d] [%-25.25s] is UP\r\n", devId
                                  , deviceInfo[devId].prevState
                                  , deviceInfo[devId].state
                                  , ipBuff);
  else if (deviceInfo[devId].state <= -2 && deviceInfo[devId].prevState <= -2)
  { 
    if (!strncasecmp(deviceInfo[devId].Descr, ipBuff, 9)==0)
    {
      DebugTf("[%03d][%2d/%2d] [%-25.25s] is DOWN\r\n", devId
                                  , deviceInfo[devId].prevState
                                  , deviceInfo[devId].state
                                  , ipBuff);
    }
  }
  else  //-- state is somewhere "in-between"
  {
    if (deviceInfo[devId].prevState > deviceInfo[devId].state)
    {
        DebugTf("[%03d][%2d/%2d] [%-25.25s] UP->DOWN\r\n", devId
                                                    , deviceInfo[devId].prevState
                                                    , deviceInfo[devId].state
                                                    , deviceInfo[devId].Descr);
    }
    else if (deviceInfo[devId].prevState < deviceInfo[devId].state)
    {
        DebugTf("[%03d][%2d/%2d] [%-25.25s] DOWN->UP\r\n", devId
                                                    , deviceInfo[devId].prevState
                                                    , deviceInfo[devId].state
                                                    , deviceInfo[devId].Descr);
    }
    else //-- strange, should never happen!
    {
      DebugTf("[%03d][%2d/%2d] [%-25.25s] state unknown\r\n", devId
                                                    , deviceInfo[devId].prevState
                                                    , deviceInfo[devId].state
                                                    , deviceInfo[devId].Descr);
    }

    //-- state has changed!
    if (deviceInfo[devId].prevState != deviceInfo[devId].state)
    {
      //-- OK, state has changed (prevState != state) to "Up" or "Down"
      if (deviceInfo[devId].state <= -2 || deviceInfo[devId].state >= 2)
      {
        //-- state is either changed to -2 (Down) or +2 (Up) --
        DebugTf("[%03d][%2d/%2d] [%-25.25s] state changed!\r\n", devId
                                          , deviceInfo[devId].prevState
                                          , deviceInfo[devId].state
                                          , ipBuff);
#ifdef _USE_TELEGRAM
        String stat = "";
        if ( (strncasecmp("No Name", deviceInfo[devId].Descr, 7)==0) )
              stat += String(ipBuff);
        else  stat += String(deviceInfo[devId].Descr);
        stat +=" state changed to ";
        if      (deviceInfo[devId].state <= -2) stat += "*Off*line";
        else if (deviceInfo[devId].state >=  2) stat += "*On*line";
        else  /* somewhere in-between */        stat += "*error*";
        if ((deviceInfo[devId].state <= -2) || (deviceInfo[devId].state >= 2))
        {
          DebugTln("Send telegram:");
          Debugf("[%s]\r\n", stat.c_str());
          bot.sendMessage(thisChatId, stat, "Markdown");
        }
#endif
        //-- update devices.csv
        if (readDeviceId(devId) == devId)
        {    
          if ( !(strncasecmp("No Name", cDescr, 7)==0) )
                writeDeviceId(devId, cDescr, deviceInfo[devId].state);
          else  writeDeviceId(devId, "No Name", deviceInfo[devId].state);
        }
        else  writeDeviceId(devId, "No Name", deviceInfo[devId].state);
      } // state -2 || +2
    } //  prevState != state

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
    //DebugTf("[%03d][%2d/%2d] [%-25.25s] Known Device\r\n", i
    //                                                , deviceInfo[i].prevState
    //                                                , deviceInfo[i].state
    //                                                , deviceInfo[i].Descr);
    pState = pingDevice(i, 5);
    
    if (pState <= 0)  
          DebugTf("[%03d][%2d/%2d] [%-25.25s] is Offline\r\n", i
                                                       , deviceInfo[i].prevState
                                                       , deviceInfo[i].state
                                                       , deviceInfo[i].Descr);
    else if (pState < 2)            
          DebugTf("[%03d][%2d/%2d] [%-25.25s] state is unknown\r\n", i
                                                      , deviceInfo[i].prevState
                                                      , deviceInfo[i].state
                                                      , deviceInfo[i].Descr);
  }
  i++;

  //-- skip to next Up device
  while((i < 255) && (strncasecmp("No Name", deviceInfo[i].Descr, 7)==0))
  {
    i++;
    yield();
  }
  if (i >= 255) i = 1;
  //DebugTf("[%03d] next 'Known Device' [%-25.25s]\r\n", i, deviceInfo[i].Descr);
  return i;
  
} //  pingKnownDevice()
