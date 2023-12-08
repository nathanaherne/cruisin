////////////////////////////////////////////////////////////////////////
// USER CONFIGURABLE SETTINGS FOR CRUISIN BRAIN
////////////////////////////////////////////////////////////////////////
// User definable variables while driving

// Reverse motor direction
boolean reverseMotorDirection = true; // true OR false

///////////////////////////////////////////////
// Throttle
// normal = normal car throttle: essentially any analog output, even a momentary switch
// joystick = forward joystick - forward, middle - stop, reverse joystick = reverse
// ppm = ppm rc input
// pwm = pwm rc input
String throttleType = "Normal";

// Speed and Acceleration settings
int maxForwardPercent = 75; // Maximum percentage of full speed while going Forward
int forwardRampPercent = 50; // Forward acceleration percent -> larger value = slower acceleration

int maxReversePercent = 40; // Maximum percentage of full speed while going Reverse
int reverseRampPercent = 50; // Reverse acceleration percent -> larger value = slower acceleration

// Brake Type
// Manual or Automatic
String brakeType = "Automatic";
int brakeRampPercent = 10; // Brake acceleration percent -> larger value = slower acceleration

///////////////////////////////////////////////
// Cruise Control 
// Off, Throttle, Button
String cruiseControlType = "Throttle"; // Throttle sets cruiseControl by time at full throttle, Button sets cruiseControl on push

int maxCruiseControlPercent = 85; // Maximum Cruise Control forward speed (percentage)
int cruiseControlRampPercent = 50; // Cruise Control acceleration speed (milliseconds), larger value = slower acceleration

// These are only required while cruiseControlType is "Throttle"
int cruiseControl_MC_throttle_EnableDelaySec = 30; // Seconds MC_throttle must be true for Cruise Control to activate
int cruiseControl_MC_throttle_DisableDelaySec = 5; // Seconds after CruiseControl enabled before MC_throttle can disable it
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// POWER STEERING
// off, ppm, pwm - this is not implemented
#define off // MANUAL CONTROL

////////////////////////////////////////////////////////////////////////
// Motor Controller Options
// Options are:
// 1. MOT_SYREN50 - Dimension Engineering Syren50 motor controller
// 2. MOT_SABER2x32 - Dimension Engineering Sabertoo2th 2x32 motor controller
// 3. MOT_SPARK - Rev Robotics Spark Motor Controller
#define MOT_SYREN50
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// Board Options
// 1. BOARD_TEENSY3
// 2. BOARD_MAPLEMINI
// 3. BOARD_ESP32
// 4. BOARD_BLACKPILL
#define BOARD_BLACKPILL
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// MANUAL CONTROL MAPPING
// Changes to these requires a restart
#include "pins.h"
const int *MC_throttle = &MC_throttle_pin;
//const int *MC_reverse = &MC_forward_pin;
const int *MC_reverseEnable = &MC_reverse_pin;
const int *MC_cruiseControl = &MC_cruiseControl_pin;
const int *MC_brake = &MC_brake_pin;

const int *MC_horn = &MC_horn_pin; // Horn (digital)
const int *MC_indicL = &MC_indicL_pin; // Indicator1 (digital)
const int *MC_indicR = &MC_indicR_pin; // Indicator2 (digital)
const int *MC_head = &MC_head_pin; // Headlights (digital)
const int *MC_hazard = &MC_hazard_pin; // Other (analog)
const int *MC_other1 = &MC_other1_pin; // Other (analog)
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Battery & current - this is not implemented
int batteryVoltageWarningPercent = 20; // voltage at which it shows a warning
float lipoCellCount = 6;
int currentWarningAmps = 20;
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// DO NOT CHANGE THINGS BELOW HERE

////////////////////////////////////////////////////////////////////////
// Lithium battery settings - this is not implemented
const float lipoCellVoltMax = 4.2;
const float lipoCellVoltMin = 3.7;
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Throttle Values
const int throttleDeadband_min_Percent = 5; // Deadband at the bottom of the throttle input
const int throttleDeadband_max_Percent = 1; // Deadband at the top of throttle input
const int throttleRangeMs = 25500; // Max milliseconds between full forward and full reverse
const int minCruiseControlPercent = 50; //Throttle percent above which Cruise Control can be enabled by button
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Enable or Disable debugging output - turn off when not being used -> Serial print on Arduino is blocking
#define DEBUG_ON
// OLED -> OLED connected via serial connection
// #define OLED
// HTTP WEBSERVER
//#define HTTP
// Inhibit Pin Ignore
#define INHIBIT_IGNORE
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Button Debounce settings
const int debounceSampleFrequencyHz = 1000; // Frequency that digital inputs checked
const int debounceMs = 20; // Number of Milliseconds to check for
////////////////////////////////////////////////////////////////////////
