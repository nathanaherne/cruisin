////////////////////////////////////////////////////////////////////////
// DO NOT EDIT THIS FILE
////////////////////////////////////////////////////////////////////////

// Get Normal Throttle and associated Inputs
void getNormalThrottleInput(int &throttleInput) {

  // Change pin settings for Maple Mini
  #if defined(BOARD_MAPLEMINI)
    pinMode(MC_throttle_pin, INPUT_ANALOG);
  #endif
  
  throttleInput = analogRead(*MC_throttle);
  
}

// Get any digital inputs for Manual Control
void getInputsDigital(int debounceMax, 
        boolean &throttleInputDigital, int &throttleInputDigitalIntegrator,
        boolean &reverseEnableInput, int &reverseEnableInputIntegrator, 
        boolean &brakeInput, int &brakeInputIntegrator,
        boolean &cruiseControlInput, int &cruiseControlInputIntegrator,
        unsigned long currentMicros, unsigned long manualControlDebounceInterval, unsigned long prev_manualControlDebounceInterval) {

  // Get Digital Values and debounce them
  if ((currentMicros - prev_manualControlDebounceInterval) >= manualControlDebounceInterval) {
    #if defined(BOARD_MAPLEMINI)
      pinMode(MC_throttle_pin, INPUT);
    #endif
    debounce(debounceMax, digitalRead(*MC_throttle), throttleInputDigital, throttleInputDigitalIntegrator);
    debounce(debounceMax, digitalRead(*MC_reverseEnable), reverseEnableInput, reverseEnableInputIntegrator);
    debounce(debounceMax, digitalRead(*MC_brake), brakeInput, brakeInputIntegrator);
    debounce(debounceMax, digitalRead(*MC_cruiseControl), cruiseControlInput, cruiseControlInputIntegrator);
    prev_manualControlDebounceInterval = currentMicros;
  }
  
}

// Cleanup Inputs for throttle deadbands etc and return CommandValues
void cleanupInputs(int throttleInput, boolean throttleInputDigital, boolean reverseEnableInput, boolean brakeInput, boolean cruiseControlInput,
        int &forwardCommand, int &reverseCommand, boolean &brakeCommand, boolean &noMotionCommanded,
        boolean &cruiseControlOn, int throttleDeadband_min, int throttleDeadband_max) {

  // Brake Commanded -> hard stop available using inhibit_drive_pin
  if (brakeInput == true) {
    brakeCommand = brakeInput; // To be expected
    forwardCommand = 0;
    reverseCommand = 0;
    noMotionCommanded = false;
    return; // ok to return here
  }
  // Forward enabled by throttle and Reverse enabled by reverseEnable
  else if (throttleType == "Normal") {
    // Forward commanded
    if (reverseEnableInput == false) {
      // Remove the throttleDeadband_min
      if (throttleInput - throttleDeadband_min < 0) {
        forwardCommand = 0;
      }
      // Remove throttleDeadband_max
      else if (throttleInput + throttleDeadband_max >= 1023) {
        forwardCommand = 1023;
      } else {
        forwardCommand = throttleInput;
      }
      reverseCommand = 0;
      brakeCommand = false;
      noMotionCommanded = false;
    }
    // Reverse commanded
    else if (reverseEnableInput == true) {
      // Remove throttleDeadband_min
      if (throttleInput - throttleDeadband_min < 0) {
        reverseCommand = 0;
      }
      // Remove throttleDeadband_max
      else if (throttleInput + throttleDeadband_max >= 1023) {
        reverseCommand = 1023;
      } else {
        reverseCommand = throttleInput;
      }
      forwardCommand = 0;
      brakeCommand = false;
      noMotionCommanded = false;
    }
  }
  
  // Catch where forwardCommand or reverseCommand is not larger than throttleDeadband_min
  if (forwardCommand == 0 && reverseCommand == 0) {
    forwardCommand = 0;
    reverseCommand = 0;
    brakeCommand = false;
    noMotionCommanded = true;
    return;
  }
}
