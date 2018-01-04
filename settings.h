/*********************************************************************
MOTOR CONTROLLER OPTION
Options are:
 1. SpiPot5v - Mobility scooter controller with 5v Pot
 2. SpiPot12v - Mobility scooter controller with 12v Pot
 3. Syren50 - Dimension Engineering Syren50 motor controller
 4. Sabertooth2x32 - Dimension Engineering Sabertooth 2x32 motor controller
*/
#define SpiPot5v
/*********************************************************************/

// Reverse motor direction
boolean reverseMotorDirection = 0; //1 = true, 0 = false

// Turn on/off debugging output
#define Debug

// Enable/Disable Cruise Control
boolean enableCruiseControl = 1;

// Speed and Acceleration settings
int maxForwardPercent = 68; // Maximum forward throttle (percentage)
int forwardRampPercent = 50; // Forward acceleration percent -> larger value = slower acceleration

int maxReversePercent = 60; // Maximum reverse throttle (percentage)
int reverseRampPercent = 50; // Reverse acceleration percent -> larger value = slower acceleration

int brakeRampPercent = 15; // Brake acceleration percent -> larger value = slower acceleration

int maxCruiseControlForwardPercent = 73; // Maximum Cruise Control forward throttle (percentage)
int cruiseControlForwardRampPercent = 50; // Cruise Control acceleration speed (milliseconds), larger value = slower acceleration
int cruiseControlForwardWaitSec = 30; // Seconds Forward must be commanded for Cruise Control to activate (1000ms = 1 second)
int cruiseControlRightButtonDisableDelaySec = 5; // Seconds after CruiseControl enabled before Right momentary button CAN disable it

/**********************************************************************************************************************
CONFIGURATION FOR 5V DIGITAL POTENTIOMETER VIA SPI
**********************************************************************************************************************/
#ifdef SpiPot5v

  // Interface with digital potentiometer via SPI
  #include <SPI.h>
  byte address = 0x00;
  // SPI pins
  const int CS = 10; // (SS) or Chip Select,
  // 11 (MOSI)
  // 12 (MISO)
  // 13 (SCK)

  // Pot output Values
  const int throttleMin = 1; // Potentiometer minimum or Reverse value
  const int brakeValue = 128; // Potentiometer Brake value
  const int throttleMax = 255; // Potentiometer Max or Forward value

#endif
/**********************************************************************************************************************/

/**********************************************************************************************************************
CONFIGURATION FOR DIMENSION ENGINEERING SYREN50 MOTOR CONTROLLER
**********************************************************************************************************************/
#ifdef Syren50

  #include <Sabertooth.h>
  #include <SoftwareSerial.h>
  
  // Throttle Values for Syren50 and Sabertooth2x32
  const int throttleMin = -127;  // Syren32 minimum or Reverse value
  const int brakeValue = 0; // Syren32 Brake value
  const int throttleMax = 127; // Syren32 maximum or Forward value
  
  const int SwSerialTxPin = 11; // Pin used for Software Serial
  
  SoftwareSerial SabertoothSWSerial(NOT_A_PIN, SwSerialTxPin); // RX on no pin (unused), TX to S1 of Syren
  Sabertooth ST(128, SabertoothSWSerial); // Address 128, and use SWSerial as the serial port.
  const int deControllerTimeout = 1000;

#endif

/***************************************************************************************************************************************
 * DO NOT CHANGE THE BELOW SETTINGS UNLESS YOU REALLY KNOW WHAT YOU ARE DOING
***************************************************************************************************************************************/

// Momentary button input pins
const int rightButton = 5; // right momentary button pin
const int leftButton = 7; // left momentary button pin

int maxForward;
int maxReverse;
int maxCruiseControlForward;

int forwardRampInterval;
int reverseRampInterval;
int brakeRampInterval;
int cruiseControlForwardRampInterval;

// Forward
//unsigned long previousForwardRampMillis = 0; // the last time Forward speed was updated

// Cruise Control
//boolean cruiseControlOn = 0; // 1 = true, 0 = false
//unsigned long cruiseControlMillis = 0; // the first time Forward first commanded
//unsigned long previousCruiseControlRampMillis = 0; // the last time Forward speed was updated

// Reverse configuration
//unsigned long previousReverseRampMillis = 0; // the last time Reverse speed was updated
//unsigned long previousBrakeRampMillis = 0; // will store last time brake throttle was updated

// current millis value
//unsigned long currentMillis;

//int currentThrottle = brakeValue; // The current throttle value - initialise as brake
