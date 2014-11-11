LED-alert
=========
(c) 2014 John Todd jtodd@loligo.com

Arduino and Total Control Lighting LED controller with web server API.

Uses the Arduino tiny computing platform to turn banks of LEDs on, off, or blinking 
with an easy-to-generate URL control method. Banks are defined hardcoded in the sketch.

For example:
 bank1 to solid red:
 
  http://1.2.3.4/?bank1-1=red
 
 Blink bank1 between red and green:
 
  http://1.2.3.4/?bank1-1=red&delay1-1=1000&bank1-2=green&delay1-2=1000

Rapid flash bank1 red and bank2 solid green

  http://1.2.3.4/?bank1-1=red&delay1-1=200&bank1-2=black&delay1-2=200&bank2-1=green

This uses the Total Control Lighting LED Pixels
and control shield.  See http://www.coolneon.com/
for hardware and https://github.com/CoolNeon/arduino-tcl
for the TCL Arduino drivers by Christopher De Vries
