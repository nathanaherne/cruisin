////////////////////////////////////////////////////////////////////////
// USER CONFIGURABLE SETTINGS FOR CRUISIN BRAIN
////////////////////////////////////////////////////////////////////////
// User definable variables while driving

///////////////////////////////////////////////
// Throttle
// Options are:
// normal = normal car throttle: essentially any analog or digital input, even a momentary switch
// joystick = forward joystick - forward, middle - stop, reverse joystick = reverse
// ppm = ppm rc input -> throttle & steering (if POWER STEERING enabled)
// pwm = pwm rc input
String throttleType = "Normal";

// Speed and Acceleration settings
// Options are: between 0 & 100
int maxForwardPercent = 75; // Maximum percentage of full speed while going Forward
int forwardRampPercent = 50; // Forward acceleration percent -> larger value = slower acceleration

int maxReversePercent = 40; // Maximum percentage of full speed while going Reverse
int reverseRampPercent = 50; // Reverse acceleration percent -> larger value = slower acceleration

// Brake Type
// Options are: 
// 1. Manual
// 2. Automatic
String brakeType = "Automatic";
// Brake acceleration percent required for all brake types.
int brakeRampPercent = 10; // Brake acceleration percent -> larger value = slower acceleration

///////////////////////////////////////////////
// Cruise Control 
// Options: off, Throttle, Button
String cruiseControlType = "Throttle"; // Throttle sets cruiseControl by time at full throttle, Button sets cruiseControl on push

int maxCruiseControlPercent = 85; // Maximum Cruise Control forward speed (percentage)
int cruiseControlRampPercent = 50; // Cruise Control acceleration speed (milliseconds), larger value = slower acceleration

// These are only required while cruiseControlType is "Throttle"
int cruiseControl_MC_throttle_EnableDelaySec = 30; // Seconds MC_throttle must be true for Cruise Control to activate
int cruiseControl_MC_throttle_DisableDelaySec = 5; // Seconds after CruiseControl enabled before MC_throttle can disable it
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Drive Motor Controller Options
// Options are:
// 1. MOT_SYREN50 - Dimension Engineering Syren50 motor controller
// 2. MOT_SABER2x32 - Dimension Engineering Sabertooth2x32 motor controller
// 3. MOT_SPARK - Rev Robotics Spark Motor Controller
#define MOT_SYREN50
// Must use the same controller for all Drive Motor Configurations below
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// Drive Motor Configuration Options -> Not implemented
// Options are:
// 1. Single - single motor for rear wheels or MOT_SABER2x32 for 2 rear motors
// 2. 2x2 - motors on two rear wheels using two separate motor controllers
// 4. 4x4 - motors on four wheels using four separate motor controllers
#define MOT_SINGLE
boolean reverseMotorDirection = true; // true OR false - Single option only
// Below not implemented
boolean reverseMotorDirectionRR = true; // true OR false - Rear Right - 2x2 & 4x4 options only
boolean reverseMotorDirectionRL = true; // true OR false - Rear Left - 2x2 & 4x4 options only
boolean reverseMotorDirectionFR = true; // true OR false - Front Right - 4x4 option only
boolean reverseMotorDirectionFL = true; // true OR false - Front Left - 4x4 option only
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// Power steering Motor Controller Options -> Not implemented
// Options are:
// 1. POW_OFF - No Power steering
// 2. POW_SYREN50 - Dimension Engineering Syren50 motor controller
// 3. POW_SPARK - Rev Robotics Spark Motor Controller
#define POW_OFF
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
// Changes to these require a restart of the board
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
// Lithium battery settings -> this is not implemented
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
// Inhibit Pin Ignore - Inhibit Pin has Cruisin Brain send brake command to motor controllers and sits in infinite loop
#define INHIBIT_IGNORE
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// Button Debounce settings
const int debounceSampleFrequencyHz = 1000; // Frequency that digital inputs checked
const int debounceMs = 20; // Number of Milliseconds to check for
////////////////////////////////////////////////////////////////////////
