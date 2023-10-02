#include <Wire.h>

// File TWI_GPS.cpp  Written by Greg Walker 01-Sep-23
// last modified 02-Oct-2023
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
  byte    bPushIndex    = 0;
  byte    bPullIndex    = 0;
  bool    bStarFound    = false;
  byte    bChecksum     = 0;

  if(m_eMode > NMEAbody ) MsgBuf[bPushIndex++] = '$';

  while (NMEAsentence[bPullIndex] > 0)
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
  MsgBuf[bPushIndex++] = 0;

  TailShift (++bPullIndex); // Step pointer to 1st byte past message
  return eReply;
}

///////////////////////////////////////////////////////////////////////////////

void TWI_GPS::TailShift(byte bTailIndex)  // Move what's left to beginning of
                                          // NMEAsentence
{
  NMEAsentence[m_bNMEAindex] = 0;
  m_eSearchState = SEARCH;
  m_bNMEAindex = 0;
  while (NMEAsentence[bTailIndex] > 0)
  {
    Shuffle(NMEAsentence[bTailIndex++]);
  }
  while(Wire.available() > 0)
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
        NMEAsentence[m_bNMEAindex++] = 0;      // and a NULL separator        
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
  byte    bChecksum = 0;
  byte    bTWIcounter = 0;
  byte    bMessageIndex = 0;
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
    *p_bIndex = 0;
    delay (1);
    Wire.beginTransmission(m_bTWIaddress);
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char TWI_GPS::read(bool* pBlockEnd)
{
  byte bResult = 0;

  if(m_bArrivedValid)
  {
    bResult = m_bArrived;
    m_bArrivedValid = false;
  }
  else bResult = Wire.read();

  if (bResult == LINE_FEED) m_bDLFpending = true;

  if (pBlockEnd != nullptr)
  {
    *pBlockEnd = true;
    if (Wire.available() > 0) *pBlockEnd = false;
  }
  return bResult;
}

///////////////////////////////////////////////////////////////////////////////

byte TWI_GPS::available()     // Actually returns zero if TWI buffer empty
{                             // and 1 if not empty
  byte bCount = 0;            // not how many bytes are held in TWI buffer

  if(m_bDLFpending)//
  {
    if(Wire.available() == 0)
    {
      Wire.requestFrom((uint8_t) m_bTWIaddress, (uint8_t) 1);
    }
    m_bArrived = Wire.read();
    if(m_bArrived == LINE_FEED)
    {
      while(Wire.available() > 0) Wire.read(); // Scrap anything else
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
      if(Wire.available() == 0)
      {
        Wire.requestFrom((int) m_bTWIaddress, (int) BUFFER_LENGTH);
      }
    }
  }
  return bCount;
}

///////////////////////////////////////////////////////////////////////////////
// Ping - Some Arduino boards have Arduino on top of Mbed and Mbed can not do a
// I2C write with zero data (to check if the Slave acknowledges to its 
// address). There is a fix to turn that into a read, but that might cause
// trouble with the GPS.

bool TWI_GPS::Ping ()
{
  bool bSuccess = false;
  byte bEndResult = 0;

  Wire.beginTransmission(m_bTWIaddress);
  bEndResult = Wire.endTransmission();
  if((bEndResult == 0) || (bEndResult == 0x04)) bSuccess = true;
  return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////

bool TWI_GPS::begin (byte bTarget)
{
	bool bReply = false;
  Wire.begin();

  if(m_bTWIaddress == 0)
  {
    if((bTarget > 0) && (bTarget <= 127))
    {
      m_bTWIaddress     = bTarget;
      bReply = Ping();
      if (bReply)
      {
        PresetMembers();
        delay(5);
      }
      else 
      {
        m_bTWIaddress     = 0;
      }
    }
  }
	return bReply;
}

///////////////////////////////////////////////////////////////////////////////

void TWI_GPS::PresetMembers()
{
  m_bDLFpending  = true;
  m_eMode        = NMEAbody;
  m_eSearchState = SEARCH;
  m_bNMEAindex   = 0;
}
///////////////////////////////////////////////////////////////////////////////

void TWI_GPS::end() { m_bTWIaddress = 0; }

///////////////////////////////////////////////////////////////////////////////

TWI_GPS::TWI_GPS() 
{ 
  m_bTWIaddress = 0; 
  PresetMembers();
}  // constructor

///////////////////////////////////////////////////////////////////////////////

void  TWI_GPS::ModeSet(NMEAmode eDemand)  { m_eMode = eDemand; }

///////////////////////////////////////////////////////////////////////////////

NMEAmode  TWI_GPS::ModeGet()  { return m_eMode; }

///////////////////////////////////////////////////////////////////////////////