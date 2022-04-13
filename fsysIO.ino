//-------------------------.........1....1....2....2....3....3....4....4....5....5....6....6....7....7
//-------------------------1...5....0....5....0....5....0....5....0....5....0....5....0....5....0....5
#define DATA_FORMAT       "%03d;%-25.25s;%3d;"
#define DATA_CSV_HEADER   "REC; NAME; STATE;"
#define DATA_RECLEN       36

#define DEV_FILE        "/devices.csv"
#define _NO_DEV_SLOTS_   (256 +1)

//--------------------------------------------------------------
void readDevices()
{
  char    rBuff[60] = {};
  int32_t bytesWritten;
  int32_t offset;

  //-test- LittleFS.remove(DEV_FILE);

  if (!LittleFS.exists(DEV_FILE)) // not found!
  {
    DebugTf("[%s] not found! Create empty one ..\r\n", DEV_FILE);

    File f = LittleFS.open(DEV_FILE, FILE_WRITE);
    DebugT('>');
    for (int w=0; w<259; w++)
    {
      Debug('.');
      yield();
      snprintf(rBuff, sizeof(rBuff), (char *)DATA_FORMAT, w, "No Name", -1);
      //Debugf("Write: %s", rBuff);
      offset = (w * DATA_RECLEN);
      f.seek(offset, SeekSet);
      //DebugTf("rec[%d], pos[%d]\r\n", w, f.position());
      bytesWritten = f.println(rBuff);
      if (bytesWritten != DATA_RECLEN)
      {
        DebugTf("ERROR! Device[%03d]: written [%d] bytes but should have been [%d]\r\n", w, bytesWritten, DATA_RECLEN);
      }
    }
    f.close();
    Debugln(" .. done!");
  }

  File f = LittleFS.open(DEV_FILE, "r+");
  if (!f)
  {
    DebugTf("Something terribly wrong opening [%s]\r\n", DEV_FILE);
    DebugFlush();
    f.close();
    delay(1000);
    return;
  }

  DebugTf("Now reading data in [%s]\r\n", DEV_FILE);
  //DebugT('<');
  for (int d=0; d<255; d++)
  {
    yield();
    //Debug('.');
    offset  = (d * DATA_RECLEN);
    if (!f.seek(offset, SeekSet))
    {
      DebugTf("Seek to [%d/%d] went wrong ..\r\n", d, offset);
    }
    //DebugTf("rec[%d] -> pos[%d]\r\n", d, f.position());
    memset(rBuff,  0, sizeof(rBuff));
    memset(cID,    0, sizeof(cID));
    memset(cDescr, 0, sizeof(cDescr));
    memset(cState, 0, sizeof(cState));
    int l = f.readBytesUntil('\n', rBuff, sizeof(rBuff));
    //DebugTf("(%d)rBuff: l[%d][%s]\r\n", d, l, rBuff);
    //rBuff[l] = 0;
    sscanf(rBuff, "%[^;];%[^;];%[^;];", cID, cDescr, cState);
    if (strlen(cDescr) == 0) sprintf(cDescr, "No Name");
    cDescr[25] = 0;
    if (  !(strncasecmp("No Name", cDescr, 7)==0) )
    {
      Debugf("Device: [%3d] -> [%s][%s]\r\n", atoi(cID), cDescr, cState);
    }
    snprintf(deviceInfo[atoi(cID)].Descr, 26, "%-25.25s", cDescr);
    deviceInfo[atoi(cID)].Descr[25] = 0;
    deviceInfo[atoi(cID)].state     = atoi(cState);
    if (deviceInfo[atoi(cID)].state >= MAX_STATE)
          deviceInfo[atoi(cID)].prevState = atoi(cState);
    else  deviceInfo[atoi(cID)].prevState = -MAX_STATE;
  }
  f.close();
  //Debugln(". done!");

} //  readDevices()


//--------------------------------------------------------------
int readDeviceId(int rID)
{
  char    rBuff[60] = {};
  int32_t bytesWritten;
  int32_t offset;

  //DebugTf("read rID[%d]\r\n", rID);

  if (!LittleFS.exists(DEV_FILE)) // not found!
  {
    readDevices();
  }

  File f = LittleFS.open(DEV_FILE, "r+");
  if (!f)
  {
    DebugTf("Something terribly wrong opening [%s]\r\n", DEV_FILE);
    f.close();
    return -1;
  }

  offset  = (rID * DATA_RECLEN);
  f.seek(offset, SeekSet);
  //DebugTf("rec[%d] -> pos[%d]\r\n", d, f.position());
  memset(rBuff,  0, sizeof(rBuff));
  memset(cID,    0, sizeof(cID));
  memset(cDescr, 0, sizeof(cDescr));
  memset(cState, 0, sizeof(cState));
  int l= f.readBytesUntil('\n', rBuff, sizeof(rBuff));
  //Debugf("rBuff: l[%d][%s]\r\n", l, rBuff);
  //rBuff[l] = 0;
  sscanf(rBuff, "%[^;];%[^;];%[^;];", cID, cDescr, cState);
  //DebugTf("Fields: [%3d/%3d] -> [%s][%s]\r\n", rID, atoi(cID), cDescr, cState);
  dState = atoi(cState);

  f.close();

  return atoi(cID);

} //  readDeviceId()


//--------------------------------------------------------------
void writeDeviceId(int cID, const char *newDescr, int newState)
{
  char    rBuff[60]  = {};
  int32_t bytesWritten;
  int32_t offset;

  if (cID == 20) DebugTf("cID[%d], newName[%s], newState[%d]\r\n", cID, newDescr, newState);

  if (cID < 1 || cID > 254)
  {
    DebugTf("deviceId must be between 1 and 254 (but is [%d])\r\n", cID);
    return;
  }
  File f = LittleFS.open(DEV_FILE, "r+");
  if (!f)
  {
    DebugTf("Something terribly wrong opening [%s]\r\n", DEV_FILE);
    f.close();
    return;
  }

  snprintf(rBuff, sizeof(rBuff), (char *)DATA_FORMAT, cID, newDescr, newState);
  if (cID == 20) DebugTf("Write: [%s]\r\n", rBuff);
  offset = (cID * DATA_RECLEN);
  f.seek(offset, SeekSet);
  //DebugTf("cID[%d], pos[%d]\r\n", cID, f.position());
  bytesWritten = f.println(rBuff);
  if (bytesWritten != DATA_RECLEN)
  {
    DebugTf("ERROR! Device[%03d]: written [%d] bytes but should have been [%d]\r\n", cID, bytesWritten, DATA_RECLEN);
  }

  f.close();
  //DebugTln("Done..");
  //DebugFlush();

} //  writeDeviceId()

/* eof */
