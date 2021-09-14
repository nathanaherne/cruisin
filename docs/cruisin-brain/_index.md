+++
title = "Cruisin Brain"
description = "An overview of cloud computing platforms and their benefits"
categories = [""]
tags = [""]

layout = "single"

draft = false
author = "Nathan Aherne"

weight = 10
first_step = "future-features/"
+++


# Cruisin Brain

The Cruisin Brain is a combination of [hardware](/cruisin-brain/index.md#hardware) and [software](/cruisin-brain/index.md#software), which when combined gives your electric car its own brain.

# Cruisin Brain Features

The Cruisin Brain was created to support the needs of different vehicles, vehicle configurations and disabled and able bodied users. It was also created to make building, modifying or upgrading an electric vehicle as cheap and safe as possible.

### Current Features

1. Control of maximum forward speed and the speed in which your vehicle accelerates when going forward.
2. Control of maximum reverse speed and the speed in which your vehicle accelerates when going in reverse.
3. Control of the speed in which your vehicle brakes.
4. The ability to enable or disable Cruise Control, as well as set it's maximum speed and acceleration.
5. The ability to work with several different general purpose motor controllers.
6. Ability to reverse motor direction if the motor(s) are running backwards.
7. Several inbuilt safety features.

[Checkout the new features we are working to add to the Cruisin Brain software and hardware.](/cruisin-brain/futureFeatures.md)

### Cruisin Brain Software

[The Cruisin Brain software is available here](https://github.com/nathanaherne/cruisin-brain)

The Cruisin Brain software is [configured in the file settings.h, available here](https://github.com/nathanaherne/cruisin-brain/blob/master/settings.h). All configuration options are detailed in this file, though we admit that without some kind of coding knowledge it may be difficult to configure.

We are currently developing a version of the Cruisin Brain that can be configured from your mobile device.

### Cruisin Brain Hardware

The Cruisin Brain is made up of several different microcontrollers (tiny computers) to maximize safety and features:

#### Mainboard

This is the main computer of the Cruisin Brain, it receives data from and manages all other Cruisin Brain microcontrollers and electronics. It is currently comprised of:

1. [Maple Mini](http://wiki.stm32duino.com/index.php?title=Maple_Mini). This is the micro-controller that runs the Cruisin Brain Education software. These boards can be purchased from Ebay for approximately $5 AUD.
2. [Wire](https://hobbyking.com/en_us/kingduino-male-female-40p-200mm-wire-jumper-cable.html) to connect the microcontroller with the motor controller, switches and power supply.
3. [5v power supply](https://hobbyking.com/en_us/micro-bec-5v-3a.html) to provide power to the Arduino Nano. If you do not buy the 5v power supply listed, make sure you buy one that can handle at least 30v input.
4. 5k resistors to pull inputs to ground
5. 0.1uf capacitor between throttle and ground

#### OLED

The Cruisin Brain OLED has its own microcontroller. The Cruisin Brain Mainboard sends all the vehicles vital information to the Cruisin Brain OLED so that it can process and display it.

<img src="https://i.imgur.com/NuHy9SM.jpg">
<br>
<br>

#### AHRS

The Cruisin Brain AHRS has its own microncontroller. It sends all of its information to the Cruisin Brain Mainboard for final processing and integration with other sensors and settings

The Cruisin Brain AHRS [is used to provide a number of new safety features](/cruisin-brain/futureFeatures.md) as well drive by wire.

### Other uses for the Cruisin Brain

The Cruisin Brain is not just for the Cruisin Electric Car but is suitable for many small electric vehicles

1. Golf Carts
2. Electric Wheelchairs
3. Robotic applications
4. Mobility scooters
5. Motor bikes
6. ATV's
7. Children's electric cars

[Read more about how you can use the Cruisin Brain in these vehicles](/cruisin-brain/otherUses.md)