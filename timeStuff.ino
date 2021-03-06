
//===========================================================================================
void printLocalTime()
{
  //struct tm timeInfo; // global declared
  getLocalTime(&timeInfo);
  if(!getLocalTime(&timeInfo))
  {
    DebugTln("Failed to obtain time");
    gotNtpTime = false;
    return;
  }
  DebugTln(&timeInfo, "%A, %B %d %Y %H:%M:%S");
  if (strlen(cStartTime) == 0)
  {
    snprintf(cStartTime, sizeof(cStartTime), "%04d-%02d-%02d %02d:%02d:%02d"
             , (timeInfo.tm_year +1900), (timeInfo.tm_mon +1), timeInfo.tm_mday
             , timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
  }
  gotNtpTime = true;

} //  printLocalTime()

//===========================================================================================
int hours()
{
  return (int)timeInfo.tm_hour;
}
int minutes()
{
  return (int)timeInfo.tm_min;
}
int seconds()
{
  return (int)timeInfo.tm_sec;
}
