# DISCLAIMER

You use this software and instructions at your own risk - if you break something, hurt someone or something or anything else happens
that is entirely on you.

<br>

-------------------------------------------------------------------------------------------------------
# OVERVIEW

Cruisin Brain software was developed for Arduino as a brain for an electric car. I initially created Cruisin Brain to run my children's custom electric cars and make them incredibly safe and performant. Cruisin Brain can be used for any electric car you wish.

The Cruisin Brain was created to receive inputs from devices controlled by the driver and sensors and turn those inputs into control commands for one or more motor controllers, plus output information to OLED Screens (dashboard) as well as things like lights.

At its most simplest the Cruisin Brain was developed to take input from two momentary switches (buttons) (named Right and Left) and translate button pushes into motion.

- Pressing the Right button, results in the car moving forwards
- Pressing both the Right and Left buttons results in the car moving reverse
- Holding the Right button for longer than 30 seconds, results in CruiseControl being enabled (car moves forward without input)
- When Cruise Control is enabled pressing the Left (at any time) or Right (5 seconds after Cruise Control is enabled) button disables Cruise Control and the car will brake
- Pressing no buttons results in the car stopping (unless Cruise Control is enabled)

It can be much more complex than this but hopefully the above gives you an idea of how Cruisin Brain works and the simplicity of it being able to take two inputs and fully control an electric car.

## WARNING ON CRUISE CONTROL
This software has a feature called Cruise Control (enabled by default can be disabled) which results in Cruisin Brain holding full throttle without input after Forward has been commanded a set amount of time (default 30 seconds but can be changed). Cruise Control can have an unintended side effect where the driver gets out of the vehicle while Cruise Control is enabled, which results in the car continuing on its merry way without a driver.

## WARNING ON USING MOBILITY SCOOTERS AS KIDS CARS
Mobility scooters are designed to carry large adults (up to 200kg), this has the benefit of one car being able to transport several children or adults at once. The negative of this is that these cars are very strong and have the ability to push a small child over or even drive over them, which I have been unable to solve programmatically. 

<br>

-------------------------------------------------------------------------------------------------------

# MOTOR CONTROLLERS SUPPORTED

This software was developed to interface with one or more external motor controllers using PWM or serial commands. Cruisin Brain can be expanded to communicate using many other communication schemes with all sorts of motor controllers. Cruisin Brain is motor controller agnostic.

## General purpose motor controllers that work with Cruisin Brain

1. Dimension Engineering Syren50 (single motor control)
2. Dimension Engineering Sabertooth2x32 (dual motor control)
3. Spark Motor Controller (single motor control)

### Syren50

Mobility scooters can have one motor with a physical differential or two motors, one motor per wheel. The Syren50 will work with both types. To use the Syren50 with one motor, just wire it as per the manual. To use the Syren50 with two motors, you should wire the motors in series so that you get an electronic differential.

NOTE: To use the Syren50 with a mobility scooter you will need to
1. remove the electronic brake attached to the end of the motor (unless you implement control of the brake using a separate piece of hardware). 
2. Cut off all proprietary plugs and connect the correct wires directly to the motor controller as per the Syren50's manual.

### Sabertooth2x32

The Sabertooth2x32 has the ability to control two electric motors (among a number of other features). This software has been designed to support the Sabertooth2x32. The main issue with using the Sabertooth2x32 with two rear motors is there will be no differential. I do not think it is a great idea to use the Sabertooth2x32 for two motors, even though it is possible. The Sabertooth2x32 is useful for people who would like to use it to control an electronic brake, though at this time, I have yet to implement or test this functionality.

The Sabertooth 2x32 has the ability to engage/disengage the electronic brake that often find on mobility scooter motors so you can leave it attached if you wish. This is a good safety feature, like a hand brake in a car but it is not required.

<br>

-------------------------------------------------------------------------------------------------------

# WIRING

The software expects the following wiring connections (the Arduino pins can be changed in settings.h):

1. Right button - digital pin 5 and Gnd on Arduino board (pushing the button grounds the pin)
2. Left button - digital pin 7 and Gnd on Arduino board (pushing the button grounds the pin)

If using Syren50 or Sabertooth2x32

1. Serial Tx to S1 on motor controller
2. 5v and Gnd on Cruisin Brain to 5v and 0v on motor controller (the Cruisin Brain can be powered by the motor controller) or an external 5v source.

If using digital potentiometer
1. Digital pin 10 on Cruisin Brain to (SS) or Chip Select pin on potentiometer chip,
2. Digital pin 11 (or MOSI) on Cruisin Brain board to MOSI pin on potentiometer chip
3. Digital pin 12 (or MISO) on Cruisin Brain to MISO pin on potentiometer chip
4. Digital pin 13 (or SCK) on  Cruisin Brain to SCK pin on potentiometer chip

You will need to wire potentiometer HIGH, potentiometer wiper and potentiometer LOW from the controller to
the corresponding pins on the digital potentiometer.

<br>

-------------------------------------------------------------------------------------------------------

# CONFIGURATION

You can configure Cruisin Brain in the settings.h file.

The top of the file provides a number of configurable options for the software, including 

1. Motor Controller options
2. Reverse motor direction (change forward motion to reverse motion)
3. Maximum Forward speed
4. Maximum Reverse speed
5. Maximum CruiseControl speed
6. Turn on/off debugging output

You can also configure acceleration (for Forward/Reverse/Cruise Control) and deceleration (braking)

<br>
