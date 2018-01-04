/***********************************************************************************************************

# OVERVIEW

Code for Arduino to turn a mobility scooter into an electric car for a child.

This software was developed to take input from two momentary switches (buttons) (named Right and Left) and translate button pushes into motion.
- Pressing the Right button, results in the car moving forwards
- Pressing both the Right and Left buttons, results in the car moving reverse
- Holding the Right button for longer than 30 seconds, results in CruiseControl being enabled (car moves forward without input)
- When Cruise Control is enabled pressing the Left button disables Cruise Control and the car will brake
- Pressing no buttons results in the car stopping

NOTE:
1. This software has a feature called Cruise Control (which can be disabled), which results the Arduino holding full throttle without input after 
Forward has been commanded a set amount of time (default 30 seconds but can be changed). This feature was created because the children I created
these cars for (7 families in our neighborhood so far) complained that pushing the button for long periods of time hurt their fingers. 
Cruise Control can have an unintended side effect (which has happened to one of the very young children) where the child falls off the car 
while Cruise Control is enabled, which results in the car continuing on its merry way without a driver.

# MOTOR CONTROLLERS SUPPORTED

This software was developed to interface with several motor controllers. IF you can justify the extra money, I recommend using a General Purpose Motor Controller 
as it makes the whole process much simpler.

## Mobility scooter controllers that have been tested (7 controllers tested do far) AND WORK digital potentiometers
1. Dynamic Controls R-Series
2. Some chinese controllers

NOTE:
1. If you find the motor controller on your donor mobility scooter does not work with digital potentiometers, you can use the wigwag from the mobility 
scooter to control forward/reverse.

Motor scooter controllers usually have a features you do not see in general motor controllers:
1. Potentiometer control of speed (5v, 12v)
2. Electronic braking system
3. Features only available via the propriatary programmer
4. Requires a 24v battery, a lower volt battery will result in the contoller not working.

The input for speed control on these controllers expect a voltage range between 0-5v, where 0v is full reverse, 2.5v is stop and 5v is full forward.
Some controllers use 0-12v, which creates problems for modern microcontrollers and digital potentiometers (most use 5v). 

Some controllers do not work with digital potentiometers which results in you not being able to control the speed programatically. 
The contollers that do not work or intermittantly work are
1. Dyanmic Controls Rhino - Blue in color (does not work with digital potentiometer)
2. PG Drives Solo - Black and Silver versions (sometimes works, sometimes not)

In my experience, Dynamic Controls Support are easier to work with than PG Drives (owned by Curtiss Wright) Support. Both companies Support people respond 
very quickly but PG Drives seem reluctant to provide much technical information about their product. Dynamic Controls on the other hand post full technical
manuals on the internet for their contollers. 

NOTE:
1. When using any mobility scooter controller, make sure you keep all the original electronics from your donor mobility scooter. 
This will allow you to get status indications from the controllers and determine the wiring the controller expects to see.
2. Having a multimeter will help immensely to work out expected wiring when using a mobility scooter motor controller.

## General purpose motor controllers
1. Dimension Engineering Syren50 (single motor control)
2. Dimension Engineering Sabertooth2x32 (dual motor control)

General purpose motor controllers are much easier to use in these applications because everything can be done in code. If you are using the Syren50
you will need to remove the electronic brake attached to the end of the motor (unless you implement control of the brake using a separate piece of hardware), 
cut off all the propriatary plugs and connect the wires directly to the motor controller as per the controllers manual. If you are using the Sabertooth2x32,
it has the ability to engage/disengage the electronic brake so you can leave it attached if you wish.

# WIRING

The software expects the following wiring (the Arduino pins can be changed in settings.h):

1. Right button - digital pin 5 and GND on Arduino board (pushing the button grounds the pin)
2. Left button - digital pin 7 and GND on Arduino board (pushing the button grounds the pin)

If using Syren50 or Sabertooth2x32

1. Serial TX (digital pin 11 on Arduino board) to S1 on motor controller
2. 5v and Gnd on Arduino to 5v and 0v on motor controller

If using digital poteniometer
1. Digital pin 10 on Arduino board to (SS) or Chip Select pin on potentiometer chip,
2. Digital pin 11 (or MOSI) on Arduino board to MOSI pin on potentiometer chip
3. Digital pin 12 (or MISO) on Arduino board to MISO pin on potentiometer chip
4. Digital pin 13 (or SCK) on Arduino board to SCK pin on potentiometer chip

You will need to wire potentiometer HIGH and potentiometer signal and potentiometer LOW from the controller to
the corresponding pins on the digital potentiometer.

# CONFIGURATION

You can configure this software in the settings.h file.

The top of the file provides a number of configurable options for the software, including 

1. Motor Controller option
2. Invert motor direction (change forward motion to reverse motion)
3. Maximum Forward speed
4. Maximum Reverse speed
5. Maximum CruiseControl speed
6. Turn on/off debugging output

You can also configure acceleration (for Forward/Reverse/Cruisecontrol) and deceleration (braking)

***********************************************************************************************************/
