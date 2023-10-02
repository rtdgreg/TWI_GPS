//  File TWI_GPS_Test.ino  Written by Greg Walker 01-Sep-23
//  Last Modified 230918
//  Test program for Interface class for MT3333 I2C GPS

#include <TWI_GPS.h>

//#define NMEA_MESSAGE_SET "PMTK104"
//#define NMEA_MESSAGE_SET "PMTK314,0,1,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1"
#define   NMEA_MESSAGE_SET "PMTK314,0,1,0,5,0"
//#define NMEA_MESSAGE_INTERVAL_MS  "PMTK220,100"
#define NMEA_MESSAGE_INTERVAL_MS  "PMTK220,100"

const byte bMaxSentence = 83; // NMEA specifies 82 max, allow one more for
                              // trailing NULL
const long MONITOR_BAUDRATE = 2000000;
const long TWI_BITRATE = 400000;  // 10000, 100000, 400000 possible
                                  // specifies TWI bits/sec
unsigned long ulMessageCount;
unsigned long ulFailCount;

TWI_GPS myGPS;

///////////////////////////////////////////////////////////////////////////////

void setup() 
{
  ulMessageCount = NULL;
  ulFailCount = NULL;

  Serial.begin(MONITOR_BAUDRATE);
  while(!Serial) delay(5);

  Serial.println("TWI_GPS_Test - starting");
  Wire.setClock(TWI_BITRATE);
  if  ( !myGPS.begin() ) { Serial.println("GPS Ping Fail"); for(;;); }
                  //  ***** Hang here forever if begin fails.  Hardware problem

  myGPS.sendMessage(NMEA_MESSAGE_SET);
  myGPS.sendMessage(NMEA_MESSAGE_INTERVAL_MS);
}

///////////////////////////////////////////////////////////////////////////////

void loop() 
{
  char  MsgBuf[NMEA_MAX_MSG];
  NMEAresult  eReply;

  while ((eReply = myGPS.receiveMessage(MsgBuf )) != NMEAoutofdata )
  {
    ReportSentence(MsgBuf, eReply);
  }
  delay(50);
}

///////////////////////////////////////////////////////////////////////////////

void ReportSentence(char Sentence[], NMEAresult eStatus)
{
  double dfFailRatePercent;

  if (eStatus != NMEAvalid) ulFailCount++;
  ulMessageCount++;

  dfFailRatePercent = ulFailCount * 100.0 / ulMessageCount;
  Serial.print(millis());
  Serial.print(",");
  Serial.print(dfFailRatePercent);
  Serial.print(",");
  Serial.print(ulFailCount);
  Serial.print(",");
  Serial.print(ulMessageCount);
  Serial.print(",");
  Serial.print((byte) eStatus);
  Serial.print(",");
  Serial.println(Sentence);
}
