This repository contains files and information about the Cruisin Brain, developed by Cruisin Co

The Cruisin Brain is a custom, drive by wire computer system for electric cars. The Cruisin Brain has been in development and use since 2017, so its safety and reliability have been well tested in a large number of scenarios and hardware configurations.

Cruisin Brain has been designed to deal with the complexities of modern electric cars, where there are many electronic systems that need to work together to create a highly performant, configurable, safe, modular electric car. I envisage a future where electric cars are not proprietary (created by Tesla or GM or Ford) but built and customised by anyone who shows an interest with the ability to upgrade components as technology changes. Having a brain that could interface with all of the electronics of modern electric cars and which is updatable would make a huge difference. Cruisin Brain could even interface with artificial intelligence systems.

The Cruisin Brain can be used for any size electric car, from go-karts built in your back yard to life size cars and everything in between.

The Cruisin Brain currently works on the STM32duino Black Pill https://stm32-base.org/boards/STM32F401CCU6-WeAct-Black-Pill-V1.2.html and it has also been developed on several different STM32 boards over the years. As each processor runs out of resources to run Cruisin Brain, I port the code over to a new, more capable processor.

I have not published any documents on how to turn a STM32duino Black Pill loaded with the Cruisin Brain code, into a fully featured Cruisin Brain. This means you will need to understand how to turn a Black Pill Board into something that you can plug wires into and control your electric car including all of its sensors and control inputs.

The Cruisin OLED was created as an example dashboard for your electric car. The code runs on a separate STM Black Pill and it listens to communications from the Cruisin Brain about its status and the status of sensors in the car. 

The Cruisin OLED then displays that information on the OLED screen, bringing vital information to the driver. The Cruisin OLED software is currently not available because I lost it. I am currently looking for it in my plethora of storage systems and I will upload it here as soon as I find it.

The Cruisin Brain was first designed as an electric brain for the custom electric cars I built for my children. The instructions on how to build an electric car from a mobility scooter for a child (or a child at heart) is here - https://nathanaherne.github.io/kids-electric-car/
