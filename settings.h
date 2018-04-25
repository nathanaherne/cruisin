/**************************************************************************************************************************************
USER SETTINGS AVAILABLE FOR THE EV SYSTEM CONTROLLER
***************************************************************************************************************************************/

// Reverse motor direction
boolean reverseMotorDirection = 0; // 1 = true, 0 = false

// Speed and Acceleration settings
int forwardMaxThrottlePercent = 62; // Maximum forward throttle percent (default 60) -> larger value = increased max speed
int forwardRampPercent = 50; // Forward acceleration percent (default 50) -> larger value = slower acceleration

int reverseMaxThrottlePercent = 35; // Maximum reverse throttle percent (default 35) -> larger value = increased max speed
int reverseRampPercent = 50; // Reverse acceleration percent (default 50) -> larger value = slower acceleration

int brakeRampPercent = 15; // Brake deceleration percent (default 15) -> larger value = slower deceleration

// Cruise Control settings
const boolean cruiseControlEnable = true; // Enable (true) Disable (false) Cruise Control
int cruiseControlMaxThrottlePercent = 72; // Maximum Cruise Control forward throttle (percentage)
int cruiseControlRampPercent = 50; // Cruise Control acceleration speed (milliseconds), larger value = slower acceleration
int cruiseControlWaitSec = 30; // Seconds Forward must be commanded for Cruise Control to activate (1000ms = 1 second)
int cruiseControlForwardInputDisableDelaySec = 5; // Seconds after CruiseControl enabled before manualInputForward CAN disable it

// Manual Control Input to Output mapping -> change requires restart
const int *manualInputForward = &inputMan1; // Must be Unique
const int *manualInputReverse1 = &inputMan1; // Reverse has two options so it can be enabled by two inputs
const int *manualInputReverse2 = &inputMan2; // Reverse has two options so it can be enabled by two inputs
const int *manualInputCruiseControl = &inputMan1; // If equals manualInputForward -> will be enabled AFTER cruiseControlWaitSec
const int *manualInputBrake = 0; // Allocate to input OR "0" (Zero) to brake when Forward or Reverse commanded

////////////////////////////////////////////////////////////////////////
// DEBUGGING
// Enable/disable debugging output
// Turn off when not being used as it increases the code timings
//#define DEBUG
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
// OLED SCREEN
//#define OLED_128x64
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// EXTERNAL CONTROL

// MANUAL CONTROL
#define INPUT_MANUAL

// STEERING WHEEL
//#define INPUT_STEERING

// You can control the vehicle via MANUAL CONTROL & REMOTE CONTROL OR REMOTE CONTROL ONLY.
// When you define MANUAL CONTROL & REMOTE CONTROL, REMOTE CONTROL overrides MANUALCONTROL

//REMOTE CONTROL -> PWM OR PWM
//#define INPUT_PWM
//#define INPUT_PPM
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// MOTOR CONTROLLER OPTIONS
//  Options are:
//  1. MC_SYREN50 - Dimension Engineering Syren50 motor controller
//  2. MC_SABER2X32 - Dimension Engineering Sabertooth 2x32 motor controller
//  3. MC_MCP4XXX - MCP4XXX family of 5v digital potentiometers
//  4. MC_PWM - PWM output to motor controller -> TODO
//  5. MC_VESC_1 - 1 x VESC brushless motor controller -> TODO
//  6. MC_VESC_2 - 2 x VESC brushless motor controller -> TODO
//  7. MC_VESC_4 - 4 x VESC brushless motor controller -> TODO
// 
#define MC_SYREN50
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// IMU SETTINGS
//  Adds IMU to be part of the control loop to:

//    1. Reduce speed in turns (ratio between angle of turn, current speed and brakeRamp)
//    2. Reduce speed or stop when driving up inclines
//    3. Reduce speed or stop when driving along inclines
//    4. Stop when large force detected (crash) -> only lateral forces
//    5. Allow you to operate a balancing vehicle   */

// IMU -> options are IMU_MPU9250
//#define IMU_MPU9250

#if defined(MPU9250)
  int reduceTurnSpeedPercent = 50; // Reduce throttle speed (forward/reverse) percentage in turns, larger value = slower deceleration
  
  int inclineUpAngle = 50; // Max incline (degrees) vehicle can drive up -> 0 (zero) disables this feature
  int inclineUpAction = 1; // 1 = reduce speed, 3 = buzzer
  int inclineUpActionStart = 30; // Percentage of inclineUpAngle where feature is enabled
  
  int inclineAlongAngle = 50; // Max incline (degrees) vechile can drive along -> 0 (zero) disables this feature
  int inclineAlongAction = 1; // 1 = reduce speed, 2 = stop, 3 = buzzer from 30% of maxInclineAlong
  int inclineAlongActionStart = 30; // Percentage of maxInclineAlong where feature is enabled
  
  boolean crashDetect = true; // Turn on crash detect feature
  int crashDetectG = 3; // Number of G's at which crashDetect is enabled
  
  int levelCorrectionDegrees = 0; // The degrees from level where the vehicle should be considered "level" -> postive (forward) or negative (reverse) value
  // For two wheel balancing, while level, the vehicle should theoretically not need to move the motors. Level should be moved to this point. A PID towards no oscillation will suit here.
#endif

////////////////////////////////////////////////////////////////////////
