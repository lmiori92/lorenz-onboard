# lorenz-onboard
Aftermarket board information system which integrates to the car's network displays and sensors

# Basic Design
The Raspberry pi's box contains everything that is required to power the microcontroller and interface it to the car's CANbus.

The connector has been chosen to be a DB-9. It contains 2x Power lines (GND + 12V), 2x MS-CAN lines, 2x HS-CAN and potentially other signals.

The 12V nominal power input features reverse polarity and short circuit protection via a diode and a low-amperage fuse (< 250mA).

The voltage is stepped down by using a chinese DC-DC converter which is very small and yet capable of supply several amps with no problems at all.
It offers current limiting, too.

# Power Supply
The Opel Astra H has a very convenient power source for an after-market device. The glove box has a timed courtesy light (the line stays active for about 15 minutes after last doorswitch toggle).
This feature is perfect for us to avoid to deplete the battery in a short time...Additionally, it's low amperage fuse guarantees us an additional protection against fire hazard.

# CANbus connection
The CANbus connections are picked from the OBD-II port which is very convenient since it exposes the 3 major CAN busses in the vehicle (if not the only ones), namely SW-CAN, MS-CAN and HS-CAN.

# Displays
The lorenz-onboard device can show information to the driver by writing text on the onboard standard display (3 lines one, but also the smaller ones) by overriding radio station information. Transport protocol is iso-tp.

Additionally it has been discovered that the small odometer display right in the middle of the tachometer and the revolutions counter can show 6-digit HEX numbers (unfortunately not discovered how to display arbitrary data), to max 3FFFFF.
This is done by sending dummy ECN (Engine Control Unit) Diagnostic Trouble Codes by just sending the standardized OBD-II response.

# Inputs
The CD-30 radio streams button presses to the MS-CAN bus. They can be captured to build a nice user interface. The protocol between the display and the radio has been partially understood e.g. to display radio input.

# The case and the electronics


[[https://github.com/lmiori92/lorenz-onboard/blob/master/doc/images/prototype.jpg|alt=design-proto]]
