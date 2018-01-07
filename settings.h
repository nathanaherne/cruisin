/*********************************************************************
MOTOR CONTROLLER OPTIONS
Options are:
 1. MCP4XXX - Mobility scooter controller or controller that requires an MCP4XXX family digital potentiometer
 2. SpiPot12v - ToDo
 3. Syren50 - Dimension Engineering Syren50 motor controller
 4. Sabertooth2x32 - Dimension Engineering Sabertooth 2x32 motor controller
*/
#define MCP4XXX
/*********************************************************************/

// Turn on/off debugging output - turn off when not being used as it increases the code timings
//#define Debug
/*********************************************************************/

// Reverse motor direction
boolean reverseMotorDirection = 1; // 1 = true, 0 = false

// Enable/Disable Cruise Control
boolean enableCruiseControl = true;

// Speed and Acceleration settings
int maxForwardPercent = 67; // Maximum forward throttle (percentage)
int forwardRampPercent = 50; // Forward acceleration percent -> larger value = slower acceleration

int maxReversePercent = 60; // Maximum reverse throttle (percentage)
int reverseRampPercent = 50; // Reverse acceleration percent -> larger value = slower acceleration

int brakeRampPercent = 15; // Brake acceleration percent -> larger value = slower acceleration

int maxCruiseControlPercent = 70; // Maximum Cruise Control forward throttle (percentage)
int cruiseControlRampPercent = 50; // Cruise Control acceleration speed (milliseconds), larger value = slower acceleration
int cruiseControlWaitSec = 30; // Seconds Forward must be commanded for Cruise Control to activate (1000ms = 1 second)
int cruiseControlRightButtonDisableDelaySec = 5; // Seconds after CruiseControl enabled before Right momentary button CAN disable it

/***************************************************************************************************************************************
 * DO NOT CHANGE THE BELOW SETTINGS UNLESS YOU REALLY KNOW WHAT YOU ARE DOING
***************************************************************************************************************************************/

/**********************************************************************************************************************
CONFIGURATION FOR 5V DIGITAL POTENTIOMETER VIA SPI
**********************************************************************************************************************/
#if defined(MCP4XXX)

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
#if defined(Syren50) || defined(Sabertooth2x32)

  #include <Sabertooth.h>
  #include <SoftwareSerial.h>
  
  // Throttle Values for Syren50 and Sabertooth2x32
  const int throttleMin = -127;  // Syren50 minimum or Reverse value
  const int brakeValue = 0; // Syren50 Brake value
  const int throttleMax = 127; // Syren50 maximum or Forward value
  
  const int SwSerialTxPin = 11; // Pin used for Software Serial
  
  SoftwareSerial SabertoothSWSerial(NOT_A_PIN, SwSerialTxPin); // RX on no pin (unused), TX to S1 of Syren
  Sabertooth ST(128, SabertoothSWSerial); // Address 128, and use SWSerial as the serial port.
  const int deControllerTimeout = 1000;

#endif

/***************************************************************************************************************************************
 * GLOBAL VARIABLES
***************************************************************************************************************************************/

// Momentary button input pins
const int rightButton = 7; // right momentary button pin
const int leftButton = 5; // left momentary button pin

int maxForward;
int maxReverse;
int maxCruiseControl;

int forwardRampInterval;
int reverseRampInterval;
int brakeRampInterval;
int cruiseControlRampInterval;
