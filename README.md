# TWI_GPS

GPS message API wrapper library for I2C Arduino connection

This software provides a small footprint, reliable, high performance applications programming interface for software executing on Atmega328 based Arduino microcontrollers.

At the interface, it delivers validated NMEA compliant report messages and accepts NMEA compliant command messages.  NMEA 0183 is a messaging structure first defined by the National Marine Electronics Association in 1983. NMEA 0183 messages are of the form:-

<$>,<message body>,<*>,<XX>,<crlf>

Where:-

 <$> is the unique ASCII dollar character

<*> is the unique ASCII asterisk character

<XX> is a two character representation of the 8 bit checksum of the message body

<crlf> is the two non printing ASCII characters, carriage return and line feed.

<message body> is the meaningful payload, subject to other format constraints that are not addressed by this library.

A valid message is limited to 80 visible characters, thereby limiting a message body to 76 characters.

This library defines a class that makes use of the Arduino Wire library, but is not derived from it.

Whilst a popular choice to connect a GPS to an Arduino is by way of an asynchronous serial interface, this library supports connection via the optional I2C (Inter Integrated Circuit) or TWI (Two Wire Interface) bus.  Two names for the same thing.

In the interests of maximising performance, this library does not make any use of heap storage.

To use this library:-

1.    Install the TWI_GPS library - available from GitHub.

2.    Insert the line "#include <TWI_GPS.h>" near the top of your sketch code.

3. declare an instance of the TWI_GPS class in the preamble to setup() or the module you have selected to handle the GPS.

        TWI_CLASS    myGPS;

4.

[![SparkFun GPS Breakout - XA1110 (Qwiic)](https://cdn.sparkfun.com//assets/parts/1/2/3/4/0/14414-SparkFun_GPS_Breakout_-_XA1110__Qwiic_-01.jpg)](https://www.sparkfun.com/products/14414)]

The MediaTek MT3333 GPS receiver offers an I2C interface for delivery if its 
report messages and to accept command instructions from its host.

Whilst it 
