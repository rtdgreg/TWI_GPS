# TWI_GPS

TWI_GPS is a GPS message API wrapper library for I2C Arduino connection.

This software provides a small footprint, reliable, high performance applications programming interface for software executing on Atmega328 based Arduino microcontrollers. 

At the interface, it delivers validated NMEA compliant report messages and accepts NMEA compliant command messages.  NMEA 0183 is a messaging structure first defined by the **National Marine Electronics Association** in 1983. NMEA 0183 messages are of the form:-

"dollar"|"body"|"star"|"XX"|"crlf" where "dollar" is the fixed visible "dollar" start character, "body" is the core payload of the message, "star" is the fixed visible "asterisk" end of body indicator character, "XX" is a two character hexadecimal checksum indicator, and "crlf" is the fixed non visible carriage control charater sequence, "carriage return" followed by "line feed".

It is implemented as a c++ class of type TWI_GPS

## Installation

TWI_GPS is available as a Zip file, in a format that meets requirements for libraries supported by Arduini IDE library manager.

Available on GitHub from https://github.com/rtdgreg/TWI_GPS

## Usage

### Principal Methods

The two key methods are **TWI_GPS::receiveMessage()** and **TWI_GPS::sendMessage()**, but before I describe these, I would like to introduce an enumerate of type **NMEAresult**.  This can take the following values:-

**NMEAresult.NMEAoutofdata** - signifies no further messages available from the GPS

**NMEAresult.valid** - a properly formed result message with correct checksum

**NMEAresult.checkfail** - a properly formed message result with incorrect checksum

**NMEAresult.overrun** - leading $ followed by 77 characters without trailing *

NMEA defines a message limit of 80 visible characters which includes a leading $, up to 76 message body characters followed by an * end of body marker and two checksum bytes.  Additionally, a valid sentence includes the non visible characters "carriage return" and "line feed", making a total record size of 82 bytes.  By convention, valid strings have a NULL byte appended - so 83 bytes of storage need to be allocated to hold the largest possible valid NMEA sentence.  This library defines the constant **NMEA_MAX_MSG** as this number.

Whilst the most humble of Arduinos provides a paltry 2k bytes of working RAM, the development environment does support dynamic memory (heap) management.  However, this class is focussed on delivering a small footprint and high performance, so  heap use has been avoided.

Therefore, the client code is charged with delivering a buffer to our library for the library to populate with the next available NMEA sentence report provided by the client code.  It is the responsibility of the client to reserve a buffer of NMEA_MAX_MSG bytes for our TWI_GPS to populate.  The client code passes a reference to this buffer to the library when requesting the next result message.

So, we come the the first, and most important method to be provided by this library:-

**NMEAresult TWI_GPS()**

`char  MsgBuf[NMEA_MAX_MSG];`

`NMEAresult  eReply;`

`eReply = myGPS.receiveMessage(MsgBuf` );

This is a **non blocking** call.  If no messages are waiting in the queue from the GPS the method will return the result **NMEAresult.outofdata** and the client can go off and do something useful.  Should that be so, the contents of the array MsgBuf[] will not have been touched.

If however at least one complete message has been prepared by the GPS, the one at the head of the queue will be delivered to the client MsgBuf[], and the return status will be set to NMEAvalid, NMEAcheckfail or NMEAoverrun. See above for explanation.

As good coding practice it is recommended that the client should repeatedly call TWI_GPS.receiveMessage() repeatedly until status NMEAoutofdata is received.  At that point, the client code has caught up with the backlog.

**TWI_GPS::sendMessage()**

Whilst the primary role of the GPS is to deliver a stream of report messages to the client, sometimes the GPS needs to be told how to behave.  Although defaults exist, the client may want something different.  The client controls the bahaviour of the GPS by sending it commands.  These commands are in the form of NMEA sentences.

For instance, the default reporting rate of a GPS is one fix per second, or 1000 milliseconds between fixes.  If the client wants a higher or lower rate, the GPS needs to receive a sentence something like:-

**$PMTK220,100*1F\r\n**

This tells the GPS to compute fixes once every 100 milliseconds, or ten times per second. However, the client code isn't really interested in the "$ ... *XX\r\n" outer wrapper.  So the library steps in and figures that out.  The client code only needs to write the simple:-

```
TWI_GPS::sendMessage("PMTK220,100");
```

and the rest is done for you.

If we consider the population of potential users of this library, perhaps, some, but not all users just want to receive a bare message and to know it has passed the validity checks.

A message something like:-

`GPRMC,094330.000,A,3113.3156,N,12121.2686,E,0.51,193.93,171210,,,A`

would be great.  But maybe, just maybe, there are some users who might want the whole text of the message for their own reasons.  Something like:-

`$GPRMC,094330.000,A,3113.3156,N,12121.2686,E,0.51,193.93,171210,,,A*68`

would be better.  Then again, some might want to have the control characters included too.  Something like:-

`$GPRMC,094330.000,A,3113.3156,N,12121.2686,E,0.51,193.93,171210,,,A*68<CR><LF>`

Any of these options is available with this library.  However, on considering the design, it was felt that whilst some users might opt for option 1, others might opt for option 2 and others might opt for option 3, it's unlikely that any user wants to select one option at some times and a different option at others.  This is modal behaviour.  This is where another enumerate comes in.  The library defines an enumerate of type NMEAmode.  This can take the following values:-

**NMEAmode.NMEAbody** - excludes the $, *, checksum and <CR><LF>, option 1

**NMEAmode.NMEAtext** - just excludes the trailing <CR><LF>, option 2

**NMEAmode.NMEAfull** - includes everything, option 3.

By default the library delivers option 1, the core body of the message, but client code can change this with the code:-

**void TWI_GPS::ModeSet(NMEAbody);** to omit the redundant wrapper characters.

At first sight the user programmer might expect the default form to be the full visible text, including the dollar, star and checksum, but excluding the carriage control characters.  The body form was chosen as a slight incentive to adopt the bare body because this library has already done the work of validating the outer wrapper. The wrapper's job is done.  The client is encouraged to accept that and leave the parsing task to analysing the real message content.

### Ancillary Methods

There are, however a few incidental methods to deal with the general housekeeping.  We have:-

**NMEAmode TWI_GPS::ModeGet();** - the complement of ModeSet() to find out the current working mode.

**bool TWI_GPS::begin( byte bTarget);** initialises the TWI_GPS object and checks the connection through the TWI bus to the GPS. Omitting the bTarget parameter selects the default GPS device address of 0x10.  This can be overriden by specifying otherwise.  Returns true is checks fail, false if not.  This method carries out a Ping() on the GPS device and returns false if that fails, althought it could fail if already begun or invalid device address specified.

**void TWI_GPS::end();** - the complement of begin().  Performs an orderly software disconnect, allowing another client to connect.  Unlikely to be needed.

**bool TWI_GPS::Ping();** - interrogates the selected GPS device via the TWI bus and reports success (true) or failure (false).

### Minimalist Coding Example

```
#include <TWI_GPS.h>
TWI_GPS MyGPS;    // Construct the TWI_GPS object
setup()
{
    Serial.begin(2000000);
    MyGPS.begin();
    Wire.setClock(400000);
    MyGPS.sendMessage("PMTK314,0,1,0,5,0");
        // GNRMC report every fix. GNGGA report every 5 fixes
    MyGPS.sendMessage("PMTK220,100"); // Fix every 100 msec
}

loop()
{
  char  MsgBuf[NMEA_MAX_MSG];
  NMEAresult  eReply;

  while ((eReply = myGPS.receiveMessage(MsgBuf )) != NMEAoutofdata )
  {
    Serial.println(MsgBuf);
    // Do whatever with message from GPS 
  }
  // 
  delay(50);
}
```

### Rationale

It is perhaps worth considering the merits of choosing to adopt a TWI bus connected GPS receiver. 

The first reason one might think of is it eliminates the need to use up either the hardware serial lines (maybe also claimed for IDE connection), or alternative software serial lines.  Physical connection lines are a limited resource.  The I2C/TWI lines used to connect between Arduino and GPS can also be used to connect to other TWI devices.

In the opinion of the author, this benefit pales in significance to the effective use of the message buffering capability of the GPS. From observation of the MicroTek MT3333 GPS receiver can buffer several seconds worth of measurement data.  With an asynchronous serial connection, the GPS will send that to its host at a rate limited by the baud rate of the connection.  Then it becomes the respnsibility of the client code to catch all that data, or lose it.  With the TWI connection, the client can choose when to collect it from the GPS.  It doesn't have to weigh the compromise of a latency vs buffer size and baud rate.  A fast clock can deliver the data promptly, but the client has to provide the resources to catch it.  With this library, the client has the option to gather all the fix data until it's right up to date, or leave it in the GPS buffer store if it's got something more pressing to do before returning to play catch up.

### Reference

For explanation of report messages from and command messages to the Microtek MT3333 GPS receiver please to MT3333 Platform NMEA Message Specification V1.07.pdf available by Google search on the internet. Please accept my apologies for not including a hyperlink - hopefully I will get the hang of that soon!

### Maintainer

This library code, in its initial form was created wholly by Greg Walker without reference to any other already available GPS libraries.

It is maintained by Greg Walker - rtdgreg@gmail.com

Please email Greg to report any problems or make suggestions for improvement.

### Development Road Map

In its first release, this library has been designed to work on a basic Arduino nano. That should mean it will work with any Atmega328 based board.

Next step will be to support Arduino nano Every, followed by check out with Mega and Uno.  Then Arduino nano BLE 33.

Please email maintainer if you have successfully used this library with any other Arduino not listed above.  Please email maintainer if you have an Arduino not listed above, have found difficulty with using it, and would like me to add support for your chosen Arduino.

I have tried this library with I2C GPS receivers from Sparkfun and from Adafruit.  Without exception, they have worked just fine.  If you have problems integrating with an I2C GPS, please let me know.

The reader will realise this library makes no attempt to parse any of the message bodies.  The mainatainer has adopted a rather blinkered approach to this development.  The mission has been to deliver validated NMEA sentences to the client code reliably and efficiently.  Maintainer does not aspire to creating a new parser when perfectly satisfactory libraries are already available to do that.

For Arduino the [TinyGPS++ library](https://github.com/mikalhart/TinyGPSPlus) is a known proven parser.  Maintainer has confined his attentions to the stated mission of the TWI_GPS library.  As yet, no attempt has been made to feed the **TinyGPS++** library with messages delivered by TWI_GPS.  That will happen during the early life of the TWI_GPS library.  Consideration will be given to making a new parser if there is a serious prospect of providing a library that's sufficient for real world requirements and makes a significantly smaller claim on, in particular, the RAM it needs to do the job.

[*SparkFun GPS Breakout - XA1110 (Qwiic) (SEN-14414)*](https://www.sparkfun.com/products/14414)

 [![SparkFun GPS Breakout - XA1110 (Qwiic)](https://cdn.sparkfun.com//assets/parts/1/2/3/4/0/14414-SparkFun_GPS_Breakout_-_XA1110__Qwiic_-01.jpg)](https://www.sparkfun.com/products/14414)
