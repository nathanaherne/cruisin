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
Some controllers use 0-12v, which creates problems for modern microcontrollers and digital potentiometers (most use 5v). Some controllers use a 
weird hybrid of potentiometer and resistive/capacitive monitoring (Dyanmic Controls Rhino is one such controller), which results in them being unable
to be controlled via digital potentiometer (you must use a WigWag).

## General purpose motor controllers
1. Dimension Engineering Syren50 (single motor control)
2. Dimension Engineering Sabertooth2x32 (dual motor control)

General purpose motor controllers are much easier to use in these applications because everything can be done in code. If you are using the Syren50
you need to remove the electronic brake attached to the end of the motor (unless you implement control of the brake using a separate piece of hardware), 
cut off all the propriatary plugs and connect the wires directly to the motor controller as per its instructions. If you are using the Sabertooth2x32,
it has the ability to engage/disengage the electronic brake.

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
