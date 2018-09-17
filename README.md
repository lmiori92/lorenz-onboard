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

# The case and the electronics
