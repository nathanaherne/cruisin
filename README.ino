/***********************************************************************************************************

# OVERVIEW

Code to turn a mobility scooter into an electric car for a child.

This software was developed to take input from two momentary switches (buttons) (named Right and Left) and translate button pushes into motion.
- Pressing the Right button, results in the car moving forwards
- Pressing both the Right and Left buttons, results in the car moving reverse
- Holding the Right button for longer than 30 seconds, results in CruiseControl being enabled (car moves forward without input)
- When Cruise Control is enabled pressing the Left button disables Cruise Control and the car will brake
- Pressing no buttons results in the car stopping

# MOTOR CONTROLLERS SUPPORTED

This software was developed to interface with several motor controllers. IF you can afford it, I recommend you use a General Purpose Motor Controller 
as it makes things much simpler.

## Mobility scooter controllers
1. Dynamic Controls
2. PG Drives (Curtiss Wright)
3. Some chinese controllers

Motor scooter controllers usually have a features you do not see in general motor controllers:
1. Potentiometer control of speed (5v, 12v)
2. Electronic braking system
3. Features only available via the propriatary programmer
4. Requires a 24v sealed lead acid battery

The input for speed control on these controllers expect a voltage range between 0-5v, where 0v is full reverse, 2.5v is stop and 5v is full forward.
Some controllers use 0-12v, which creates problems for modern microcontrollers and digital potentiometers (most use 5v). 

Some controllers do not work with digital potentiometers which results in you not being able to control the speed programatically. 
The contollers that do not work or intermittantly work are
1. Dyanmic Controls Rhino - Blue in color
2. PG Drives Solo - Black and Silver versions

In my experience, Dynamic Controls support are much easier to work with than PG Drives (owned by Curtis Wright) support. Both companies Support people respond 
very quickly but PG Drives are seem reluctant to provide much technical support. Dynamic Controls on the other hand provide full technical manuals for their contollers.
When using either companies controllers, make sure you keep all the original electronics from your donor mobility scooter. This will allow you to get status indications
from the controllers and determine the wiring the controller expects to see.

## General purpose motor controllers
1. Dimension Engineering Syren50 (single motor control)
2. Dimension Engineering Sabertooth2x32 (dual motor control)

General purpose motor controllers are much easier to use in these applications because everything can be done in code. If you are using the Syren50
you will need to remove the electronic brake attached to the end of the motor (unless you implement control of the brake using a separate piece of hardware), 
cut off all the propriatary plugs and connect the wires directly to the motor controller as per the controllers manual. If you are using the Sabertooth2x32,
it has the ability to engage/disengage the electronic brake so you can leave it attached if you wish.

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
