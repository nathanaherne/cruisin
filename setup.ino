// Calculate variables required to run the software
void calculateGlobalVariables() {
  
  // Calculations for maxForward
  if (reverseMotorDirection) {
    maxForward = brakeValue - (((brakeValue - throttleMin) * forwardMaxThrottlePercent) / 100); // Based on maxForwardPercent & Throttle Output Value settings
  } else if (!reverseMotorDirection) {
    maxForward = brakeValue + (((throttleMax - brakeValue) * forwardMaxThrottlePercent) / 100); // Based on maxForwardPercent & Throttle Output Value settings
  } else {
    maxForward = brakeValue; // For safety
  }
  
  // Calculation for maxReverse
  if (reverseMotorDirection) {
    maxReverse = brakeValue + (((throttleMax - brakeValue) * reverseMaxThrottlePercent) / 100); // Based on maxReversePercent & Throttle Output Value settings
  } else if (!reverseMotorDirection) {
    maxReverse = brakeValue - (((brakeValue - throttleMin) * reverseMaxThrottlePercent) / 100); // Based on maxReversePercent & Throttle Output Value settings
  } else {
    maxReverse = brakeValue; // For safety
  }
  
  // Calculation for maxCCForward
  if (reverseMotorDirection) {
    maxCruiseControl = brakeValue - (((brakeValue - throttleMin) * cruiseControlMaxThrottlePercent) / 100); //Based on maxCruiseControlPercent & Throttle Output Value settings
  } else if (!reverseMotorDirection) {
    maxCruiseControl = brakeValue + (((throttleMax - brakeValue) * cruiseControlMaxThrottlePercent) / 100); // Based on maxCruiseControlPercent & Throttle Output Value settings
  } else {
    maxCruiseControl = brakeValue; // For safety
  }

  // Calculations for ramp intervals
  forwardRampInterval = (forwardRampPercent * 100) / 100; // milliseconds between Forward updates
  reverseRampInterval = (reverseRampPercent * 100) / 100; // milliseconds between Reverse updates
  brakeRampInterval =  (brakeRampPercent * 100) / 100; // milliseconds between Reverse updates
  cruiseControlRampInterval = (cruiseControlRampPercent * 100) / 100; // milliseconds between Reverse updates
  
}

// Make sure that Manual Input to Output mapping is configured correctly
void validateManualControlConfig(boolean &manualControlError) {

  // If Foward and Reverse are NOT equal
  if (manualInputForward != manualInputReverse1 && manualInputForward != manualInputReverse2 ) {
    manualControlError = 0;
  }
  
}

// Validate peripheral configuration
void validatePeripheralConfig(boolean &ppmAndPwmError, boolean &noInputError, boolean &noMotorOutputError) {

  // Check if an Control Input has been selected
  #if !defined(PWM) || !defined(PPM) || !defined(MANUAL) || !defined(STEERING)
  #else
    noInputError = 0;
  #endif

  // Check if PWM & PPM are NOT enabled at the same time
  #if !defined(PWM) && defined(PPM) || defined(PWM) && !defined(PPM)
    ppmAndPwmError = 0;
  #endif

  #if !defined(MC_SYREN50) || !defined(MC_SABER2X32) || !defined(MC_MCP4XXX)
  #else
    noMotorOutputError = 0;
  #endif
}

//T Check for errors before starting
void isError(boolean &manualControlError, boolean &noInputError, boolean &pwmAndPpmError, boolean &noMotorOutputError) {

  if (noInputError) {
    //Serial print "No control input has been enabled"
  }

  if (pwmAndPpmError) {
     // Serial print "RC Control --> PPM & PWM cannot be enabled at the same time,";
  }

  if (manualControlError) {
    // There is a manual control error
  }

  if (noMotorOutputError) {
    // There is a motor output error
  }
  
}

void setupPins() {

  // Setup output pins
  pinMode(buzzer, OUTPUT);
  pinMode(horn, OUTPUT);
  pinMode(headLights, OUTPUT);
  pinMode(brakeLights, OUTPUT);

  #if defined(MANUALCONTROL)
    // Setup the Manual Control input pins
    pinMode(inputMan1, INPUT_PULLUP);
    pinMode(inputMan2, INPUT_PULLUP);
    pinMode(inputMan3, INPUT_PULLUP);
    pinMode(inputMan4, INPUT_PULLUP);
    pinMode(inputMan5, INPUT_PULLUP);
    pinMode(inputMan6, INPUT_PULLUP);
    pinMode(inputMan7, INPUT_PULLUP);
    pinMode(inputMan8, INPUT_PULLUP);
    pinMode(inputMan9, INPUT_PULLUP);
    pinMode(inputMan10, INPUT_PULLUP);
  #endif

  #if defined(PWM)
    pinMode(inputPwmThrottle, INPUT_PULLUP);
    pinMode(inputPwmSteering, INPUT_PULLUP);
    pinMode(inputPwmSwitch, INPUT_PULLUP);
  #endif

  #if defined(PPM)
    pinMode(inputPPM, INPUT_PULLUP);
  #endif

  #if defined(MCP4XXX)
    pinMode(MCP4XXX_CS, OUTPUT);
  #endif
  
}

void setupPeripherals() {

  #if defined(MCP4XXX)
    pinMode(MCP4XXX_CS, OUTPUT);
    SPI.begin();
  #endif

  #if defined(Syren50) || defined(Sabertooth2x32)
    pinMode(SSTxPin, OUTPUT);
    SabertoothSWSerial.begin(9600);
    ST.setTimeout(deControllerTimeout); // Controller must recieve commands every X milliseconds or it will stop motors
    ST.setRamping(0); // Remove ramping in the controller as it is done in this code.
    ST.autobaud();
  #endif

  // IMU_MPU9250
  #if defined(IMU_MPU9250)
    // Setup MPU9250
  #endif

  // OLED_128x64
  #if defined(OLED_128x64)
    // Setup I2C OLED 128x64
  #endif
  
}

