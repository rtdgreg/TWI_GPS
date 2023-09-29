//  File TWI_GPS.h  Written by Greg Walker 01-Sep-23
//  Last modified 29-Sep-2023
// Interface class for MT3333 I2C GPS

#include <Arduino.h>
#include <Wire.h>

// TWI clock rates: 10,000 slow, 100,000 standard, 400,000 fast

enum  NMEAmode    { NMEAbody, NMEAtext, NMEAfull};
enum  NMEAresult  { NMEAoutofdata = 0, NMEAvalid, NMEAcheckfail, NMEAoverrun};
const byte        NMEA_MAX_MSG = 83;  // NMEA specifies 82 max for a record
                                      // That includes cr, lf & $
                                      // We'll provide an extra slot for
                                      // a null terminator
const byte        GPS_ID     = 0x10;
const char        LINE_FEED  = 0x0A;

///////////////////////////////////////////////////////////////////////////////

class TWI_GPS
{
public:

  NMEAresult  receiveMessage  (char MsgBuf[]);
  void        sendMessage     (char MsgBuf[]);

  void        ModeSet(NMEAmode eDemand = NMEAbody);
  NMEAmode    ModeGet();

  bool	      begin(byte bTarget = GPS_ID);
  void        end();
  bool        Ping();
              TWI_GPS();

private:
  enum  RUNOUT {SEARCH = 0, BODY, CKSM1, CKSM2, COMPLETE};

  byte        m_bTWIaddress;    // default 0x10 for GPS I2C address
  byte        m_bArrived;       // the byte that has arrived
  bool        m_bArrivedValid;  // true if not yet used
  bool        m_bDLFpending;    // double line feed tells us exhausted the data

  NMEAmode    m_eMode;          // message mode, bare body, visible text, 
                              // or whole NMEA sentence including cr/lf
  RUNOUT      m_eSearchState;   // progress through a sentence
  char        NMEAsentence[NMEA_MAX_MSG]; // buffer to hold a sentence, pending
                                        // collection by readMessage()
  byte        m_bNMEAindex;     // index to next slot to fill.

  const char*  HexList = "0123456789ABCDEF"; // nibble to ASCII code

  void        PutByte(byte bItem, byte* p_bIndex);
  bool        Process_Character();                    // Known to be available. 
  NMEAresult  ProcessCompletedMessage(char MsgBuf[]); // Known to be possible
  byte        available();                      // zero if TWI buffer exhausted
  char        read(bool* pBlockEnd = nullptr);  // indicates last TWI buffer
  void        TailShift(byte bTailIndex);       // residual bytes in our buffer
  void        Shuffle(byte chOneChar);          // append one byte to building
                                                // sentence

  void        PresetMembers();

};

///////////////////////////////////////////////////////////////////////////////
