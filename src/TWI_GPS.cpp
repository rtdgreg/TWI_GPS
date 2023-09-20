#include <Wire.h>

// File TWI_GPS.cpp  Written by Greg Walker 01-Sep-23
// last modified 16-Sep-2023
// Interface class for MediaTek MT3333 I2C GPS

#include <Arduino.h>
#include <TWI_GPS.h>

///////////////////////////////////////////////////////////////////////////////

NMEAresult TWI_GPS::receiveMessage(char MsgBuf[])
{
  NMEAresult  eReply = NMEAoutofdata;

  if (m_eSearchState == COMPLETE) 
  {
    eReply = ProcessCompletedMessage( MsgBuf );
  }
  else 
  {
    while (available()) 
    {
      if (Process_Character())
      {
        eReply = ProcessCompletedMessage(MsgBuf);
        break;
      }
    }
  }
  return eReply;
}

///////////////////////////////////////////////////////////////////////////////

bool TWI_GPS::Process_Character() // Known to be available.  
                                  // Returns true if end of sentence
{
  bool bReply = false;
  char  chOneChar = read();

  if(m_bNMEAindex >= (NMEA_MAX_MSG - 2)) m_eSearchState = COMPLETE;
  Shuffle(chOneChar);
  if(m_eSearchState == COMPLETE ) bReply = true;
  return bReply;
}

///////////////////////////////////////////////////////////////////////////////

NMEAresult  TWI_GPS::ProcessCompletedMessage(char MsgBuf[])
{
  NMEAresult    eReply  = NMEAoutofdata;
  byte    bPushIndex    = NULL;
  byte    bPullIndex    = NULL;
  bool    bStarFound    = false;
  byte    bChecksum     = NULL;

  if(m_eMode > NMEAbody ) MsgBuf[bPushIndex++] = '$';

  while (NMEAsentence[bPullIndex] > NULL)
  {
    if (NMEAsentence[bPullIndex] == '*') bStarFound = true;
    if (!bStarFound) bChecksum ^= NMEAsentence[bPullIndex];
    MsgBuf[bPushIndex++] = NMEAsentence[bPullIndex++];
  }

  if ((MsgBuf[bPushIndex - 3]) != '*') 
  {
    eReply = NMEAoverrun;
  }
  else
  {
    eReply = NMEAcheckfail;
    if ((HexList[bChecksum >> 4]) == MsgBuf[bPushIndex - 2])
    {
      if ((HexList[bChecksum & 0x0F]) == MsgBuf[bPushIndex - 1])
      {
        eReply = NMEAvalid;
      }
    }
  }
  if ((eReply == NMEAvalid) && (m_eMode == NMEAbody)) bPushIndex -= 3;
  if (m_eMode == NMEAfull)
  {
    MsgBuf[bPushIndex++] = '\r';
    MsgBuf[bPushIndex++] = '\n';
  }
  MsgBuf[bPushIndex++] = NULL;

  TailShift (++bPullIndex); // Step pointer to 1st byte past message
  return eReply;
}

///////////////////////////////////////////////////////////////////////////////

void TWI_GPS::TailShift(byte bTailIndex)  // Move what's left to beginning of
                                          // NMEAsentence
{
  NMEAsentence[m_bNMEAindex] = NULL;
  m_eSearchState = SEARCH;
  m_bNMEAindex = NULL;
  while (NMEAsentence[bTailIndex] > NULL)
  {
    Shuffle(NMEAsentence[bTailIndex++]);
  }
  while(Wire.available())
  {
    Shuffle(Wire.read());
  }
}

///////////////////////////////////////////////////////////////////////////////

void  TWI_GPS::Shuffle(byte chOneChar)
{
  if((chOneChar >= ' ') && (chOneChar <= '_')) // Only use ASCII 0x20 ... 0x5F
  {
    switch (m_eSearchState)
    {
      case SEARCH:
        if (chOneChar == '$') m_eSearchState = BODY;
        break;

      case BODY:
        if (chOneChar == '*')
        {
          m_eSearchState = CKSM1;
        }
        NMEAsentence[m_bNMEAindex++] = chOneChar; // to put body & star
        break;

      case CKSM1:
        NMEAsentence[m_bNMEAindex++] = chOneChar; // to put 1st checksum char
        m_eSearchState = CKSM2;
        break;

      case CKSM2:
        NMEAsentence[m_bNMEAindex++] = chOneChar; // to put 2nd checksum char
        NMEAsentence[m_bNMEAindex++] = NULL;      // and a NULL separator        
        m_eSearchState = COMPLETE;
        break;

      case COMPLETE:
        NMEAsentence[m_bNMEAindex++] = chOneChar;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void  TWI_GPS::sendMessage(char MsgBuf[])
{
  byte    bChecksum = NULL;
  byte    bTWIcounter = NULL;
  byte    bMessageIndex = NULL;
  size_t  uMessageLength = strlen(MsgBuf);
  if (uMessageLength >= NMEA_MAX_MSG - 6) uMessageLength = NMEA_MAX_MSG - 6;
  if ( uMessageLength >= 1)
  {
    Wire.beginTransmission(m_bTWIaddress);
    PutByte('$', &bTWIcounter);
    if(MsgBuf[0] == '$') ++bMessageIndex;
    while (bMessageIndex < uMessageLength)
    {
      if (MsgBuf[bMessageIndex] == '*' ) break;
      PutByte(MsgBuf[bMessageIndex], &bTWIcounter);
      bChecksum ^= MsgBuf[bMessageIndex++];
    }
    PutByte('*', &bTWIcounter);
    PutByte(HexList[bChecksum >> 4], &bTWIcounter);
    PutByte(HexList[bChecksum & 0x0F], &bTWIcounter);
    PutByte('\r', &bTWIcounter);
    PutByte('\n', &bTWIcounter);
    Wire.endTransmission();  
  }
  return;
}

///////////////////////////////////////////////////////////////////////////////

void  TWI_GPS::PutByte(byte bItem, byte* p_bIndex)
{
  Wire.write(bItem);
  if (++*p_bIndex >= BUFFER_LENGTH)
  {
    Wire.endTransmission();
    *p_bIndex = NULL;
    //delay (1);  // I have seen written that GPS needs a bit of time here, but
                  // seems to work just fine without.
    Wire.beginTransmission(m_bTWIaddress);
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char TWI_GPS::read(bool* pBlockEnd)
{
  byte bResult = NULL;

  if(m_bArrivedValid)
  {
    bResult = m_bArrived;
    m_bArrivedValid = false;
  }
  else bResult = Wire.read();

  if (bResult == LINE_FEED) m_bDLFpending = true;

  if (pBlockEnd != nullptr) *pBlockEnd = !Wire.available();
  return bResult;
}

///////////////////////////////////////////////////////////////////////////////

byte TWI_GPS::available()     // Actually returns zero if TWI buffer empty
{                             // and 1 if not empty
  byte bCount = NULL;         // not how many bytes are held in TWI buffer

  if(m_bDLFpending)//
  {
    if(Wire.available() == NULL)
    {
      Wire.requestFrom((uint8_t) m_bTWIaddress, (uint8_t) 1);
    }
    m_bArrived = Wire.read();
    if(m_bArrived == LINE_FEED)
    {
      while(Wire.available() > 0) Wire.read();
    }
    else 
    {
      m_bDLFpending = false;
      m_bArrivedValid = true;
      bCount = 1;
    }
  }
  else // we're into a message
  {
    bCount = 1;
    if(!m_bArrivedValid)
    {
      if(Wire.available() == NULL)
      {
        Wire.requestFrom((int) m_bTWIaddress, (int) BUFFER_LENGTH);
      }
    }
  }
  return bCount;
}

///////////////////////////////////////////////////////////////////////////////

bool TWI_GPS::Ping ()
{
  bool bSuccess = false;
  byte bEndResult = NULL;
  Wire.beginTransmission(m_bTWIaddress);
  bEndResult = Wire.endTransmission();
  if((bEndResult && !0x04) == NULL) bSuccess = true;
  return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////

bool TWI_GPS::begin (byte bTarget)
{
	bool bReply;
  Wire.begin();

  if(m_bTWIaddress == NULL)
  {
    if((bTarget > NULL) && (bTarget <= 127))
    {
      m_bTWIaddress     = bTarget;
      bReply = Ping();
      if (bReply)
      {
        m_bDLFpending     = true;
        m_eMode           = NMEAtext;
        m_eSearchState    = SEARCH;
        m_bNMEAindex      = NULL;
        delay(5);
      }
      else 
      {
        m_bTWIaddress     = NULL;
      }
    }
  }
	return bReply;
}

///////////////////////////////////////////////////////////////////////////////

void TWI_GPS::end() { m_bTWIaddress = NULL; }

///////////////////////////////////////////////////////////////////////////////

TWI_GPS::TWI_GPS() { m_bTWIaddress = NULL; }  // constructor

///////////////////////////////////////////////////////////////////////////////

void  TWI_GPS::ModeSet(NMEAmode eDemand)  { m_eMode = eDemand; }

///////////////////////////////////////////////////////////////////////////////

NMEAmode  TWI_GPS::ModeGet()  { return m_eMode; }

///////////////////////////////////////////////////////////////////////////////