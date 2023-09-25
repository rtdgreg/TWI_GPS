# TWI_GPS

GPS message API wrapper library for I2C Arduino connection

This software provides a small footprint, reliable, high performance applications programming interface for software executing on Atmega328 based Arduino microcontrollers.

At the interface, it delivers validated NMEA compliant report messages and accepts NMEA compliant command messages.  NMEA 0183 is a messaging structure first defined by the National Marine Electronics Association in 1983. NMEA 0183 messages are of the form:-

It is implemented as a c++ class of type TWI_GPS

The two key methods are TWI_GPS::receiveMessage() and TWI_GPS::sendMessage(), but before I describe these, I would like to introduce an enumerate of type "NMEAresult".  This can take the following values:-

NMEAresult.NMEAoutofdata - signifies no further messages available from the GPS

NMEAresult.valid - a properly formed result message with correct checksum

NMEAresult.checkfail - a properly formed message result with incorrect checksum

NMEAresult.overrun - leading $ followed by 77 characters without trailing *

NMEA defines a message limit of 80 visible characters which includes a leading $, up to 76 message body characters followed by an * end of body marker and two checksum bytes.  Additionally, a valid sentence includes the non visible characters "carriage return" and "line feed", making a total record size of 82 bytes.  By convention, valid strings have a NULL byte appended - so 83 bytes of storage need to be allocated to hold the largest possible valid NMEA sentence.  This library defines the constant NMEA_MAX_MSG as this number.

Whilst the most humble of Arduinos provides a paltry 2k bytes of working RAM, the development environment does support dynamic memory (heap) management.  However, this class is focussed on delivering a small footprint and high performance, so  heap use has been avoided.

Therefore, the client code is charged with delivering a buffer to our library for the library to populate with the next available NMEA sentence report provided by the client code.  It is the responsibility of the client to reserve a buffer of NMEA_MAX_MSG bytes for our TWI_GPS to populate.  The client code passes a reference to this buffer to the library when requesting the next result message.

So, we come the the first, and most important method to be provided by this library:-

NMEAresult TWI_GPS()

Usage:-

char  MsgBuf[NMEA_MAX_MSG];

NMEAresult  eReply;

eReply = myGPS.receiveMessage(MsgBuf );

This is a non blocking call.  If no messages are waiting in the queue from the GPS the method will return the result NMEAresult.outofdata and the client can go off and do something useful.  Should that be so, the contents of the array MsgBuf[] will not have been touched.

If however at least one complete message has been prepared by the GPS, the one at the head of the queue will be delivered to the client MsgBuf[], and the return status will be set to NMEAvalid, NMEAcheckfail or NMEAoverrun. See above for explanation.

As good coding practice it is recommended that the client should repeatedly call TWI_GPS.receiveMessage() repeatedly until status NMEAoutofdata is received.  At that point, the client code has caught up with the backlog.

The second key method is TWI_GPS::sendMessage()

Whilst the primary role of the GPS is to deliver a stream of report messages to the client, sometimes the GPS needs to be told how to behave.  Although defaults exist, the client may want something different.  The client controls the bahaviour of the GPS by sending it commands.  These commands are in the form of NMEA sentences.

For instance, the default reporting rate of a GPS is one fix per second, or 1000 milliseconds between fixes.  If the client wants a higher or lower rate, the GPS needs to receive a sentence something like:-

$PMTK220,100*1F\r\n

This tells the GPS to compute fixes once every 100 milliseconds, or ten times per second. However, the client code isn't really interested in the "$ ... *XX\r\n" outer wrapper.  So the library steps in and figures that out.  The client code only needs to write the simple:-

TWI_GPS::sendMessage("PMTK220,100");

and the rest is done for you.

If we consider the population of potential users of this library, perhaps, some, but not all users just want to receive a bare message and to know it has passed the validity checks.

A message something like:-

GPRMC,094330.000,A,3113.3156,N,12121.2686,E,0.51,193.93,171210,,,A

would be great.  But maybe, just maybe, there are some users who might want the whole text of the message for their own reasons.  Something like:-

$GPRMC,094330.000,A,3113.3156,N,12121.2686,E,0.51,193.93,171210,,,A*68

would be better.  Then again, some might want to have the controlcharacters included too.  Something like:-

$GPRMC,094330.000,A,3113.3156,N,12121.2686,E,0.51,193.93,171210,,,A*68<CR><LF>

Any of these options is available with this library.  However, on considering the design, it was felt that whilst some users might opt for option 1, others might opt for option 2 and others might opt for option 3, it's unlikely that any user wants to select one option at some times and a different option at others.  This is modal behaviour.  This is where another enumerate comes in.  The library defines an enumerate of type NMEAmode.  This can take the following values:-

NMEAmode.NMEAbody - excludes the $, *, checksum and <CR><LF>, option 1

NMEAmode.NMEAtext - just excludes the trailing <CR><LF>, option 2

NMEAmode.NMEAfull - includes everything, option 3.

By default the library delivers option 2, the visible text characters, but client code can change this with the code:-

void TWI_GPS::ModeSet(NMEAbody); to omit the redundant wrapper characters.

That deals with the core functionality of this library.  There are, however a few incidental methods to deal with the general housekeeping.  We have:-

NMEAmode TWI_GPS::ModeGet(); - the complement of ModeSet() to find out the current working mode.

bool TWI_GPS::begin( byte bTarget); initialises the TWI_GPS object and checks the connection through the TWI bus to the GPS. Omitting the bTarget parameter selects the default GPS device address of 0x10.  This can be overriden by specifying otherwise.  Returns true is checks fail, false if not.  This method carries out a Ping() on the GPS device and returns false if that fails, althought it could fail if already begun or invalid device address specified.

void TWI_GPS::end(); - the complement of begin().  Performs an orderly software disconnect, allowing another client to connect.  Unlikely to be needed.

boolTWI_GPS::Ping(); - interrogates the selected GPS device via the TWI bus and reports success (true) or failure (false).

Rationale

It is perhaps worth considering the merits of choosing to adopt a TWI bus connected GPS receiver. 

The first reason one might think of is it eliminates the need to use up either the hardware serial lines (maybe also claimed for IDE connection), or alternative software serial lines.  Physical connection lines are a limited resource.  The I2C/TWI lines used to connect between Arduino and GPS can also be used to connect to other TWI devices.

In the opinion of the author, this benefit pales in significance to the effective use of the message buffering capability of the GPS. From observation of the MicroTek MT3333 GPS receiver can buffer several seconds worth of measurement data.  With an asynchronous serial connection, the GPS will send that to its host at a rate limited by the baud rate of the connection.  Then it becomes the respnsibility of the client code to catch all that data, or lose it.  With the TWI connection, the client can choose when to collect it from the GPS.  It doesn't have to weigh the compromise of a latency vs buffer size and baud rate.  A fast clock can deliver the data promptly, but the client has to provide the resources to catch it.  With this library, the client has the option to gather all the fix data until it's right up to date, or leave it in the GPS buffer store if it's got something more pressing to do before returning to play catch up.

Reference

For explanation of report messages from and command messages to the Microtek MT3333 GPS receiver please to MT3333 Platform NMEA Message Specification V1.07.pdf available by Google search on the internet. Please accept my apologies for not including a hyperlink - hopefully I will get the hang of that soon!

To use this library:-

1.    Install the TWI_GPS library - available from GitHub.

2.    Insert the line "#include <TWI_GPS.h>" near the top of your sketch code.

3. declare an instance of the TWI_GPS class in the preamble to setup() or the module you have selected to handle the GPS.

        TWI_CLASS    myGPS;

4. Within the setup() procedure, initialise the TWI_GPS object with a call such as
   byte bResult = myGPS.begin();
   This will do the start up and assign the default device address of 0x10 for the MT3333 GPS.  By qualifying the call (eg myGPS.begin(0x20)) you can override this default.
   The method returns a boolean. true if successful.  false is an indication of a wiring error.  Successful communication has not been established.

5. Within the setup() procedure, consider the I2C clock rate.  By default it is 100kHz.  The GPS is likely to have a ceiling of 400kHz, so you don't want to set it any faster than that. Constraints such as other I2C devices sharing the bus, or the physical wiring might dictate a lower rate.  However, in the absence of such restrictions do try choosing this 400 kHz clock rate.  You can always revert to a lower figure if you experience transmission errors.  Use method Wire.setClock(400000) to set the I2C clock to 400 kHz.  This rate becomes applicable to transactions with all devices sharing the same I2C bus. 

6. Next, still within the setup() procedure, you will probably want to configure the GPS by sending it NMEA command messages.  These go to the GPS as fully formed NMEA sentences, but you only need to be concerned with the message body.  For instance, an instruction to set the rate at which the GPS computes position (etc) fixes to 10 Hz, that is equivalent to 100 milliseconds between fixes.  The command to achieve that is:-
   $PMTK220,100*1F\r\n

        In this case, the leading $ and the trailing *1F\r\n is the transport level framework, and the PMTK220,100 is the meaningful body of the message.  P for a private message, MTK for the vendor specific MediaTek identifier, 220 signifies command number 220 (set fix interval), and 100 defines the fix interval as 100 milliseconds.  The 1F is the result of a checksum computation.  The application programmer doesn't particulary want to calculate that, but the GPS requires it to validate be confident that the message it receives is what the Arduino sent.  In order to instruct the GPS to compute fixes ever 100 milliseconds, the following code could be written:-

        myGPS.sendMessage("PMTK220,100");

However, myGPS.sendMessage("$PMTK220,100*1F");

            or myGPS.sendMessage("$PMTK220,100*1F\r\n");

            or even myGPS.sendMessage("$PMTK220,100*XX"); could be written.

Essentially TWI_GPS will do its best to figure out what is wanted.  If 

The MediaTek MT3333 GPS receiver offers an I2C interface for delivery if its 
report messages and to accept command instructions from its host.

[*SparkFun GPS Breakout - XA1110 (Qwiic) (SEN-14414)*](https://www.sparkfun.com/products/14414)

 [![SparkFun GPS Breakout - XA1110 (Qwiic)](https://cdn.sparkfun.com//assets/parts/1/2/3/4/0/14414-SparkFun_GPS_Breakout_-_XA1110__Qwiic_-01.jpg)](https://www.sparkfun.com/products/14414)
