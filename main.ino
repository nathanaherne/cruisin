#include "pins.h" // pins file
#include "settings.h" // Settings file
#include "globals.h" // Global variables
#include "configPeripherals.h" // Peripheral configuration file

void setup() {

  // Set all as error and unset if no error
  boolean manualControlError = 1;
  boolean pwmAndPpmError = 1;
  boolean noInputError = 1;
  boolean noMotorOutputError = 1;

  // Setup enabled peripherals
  setupPeripherals();

  // Stop motor(s)
  sendThrottleCommand(brakeValue);

  // Check if vehicle is moving -> brake value incorrect

  // Calculate global variables
  calculateGlobalVariables();

  // Validate Manual Control Configuration
  validateManualControlConfig(manualControlError);

  // Validate Peripheral Config
  validatePeripheralConfig(pwmAndPpmError, noInputError, noMotorOutputError);

  // Check if ERROR
  isError(manualControlError, noInputError, pwmAndPpmError, noMotorOutputError);

  // Setup Input pins
  setupPins();

  #if defined(Debug)
    startupDebug();
  #endif
  
}

void loop() {

  // Variable Declarations
  static int motionCommanded; // variable to store motion commands from momentary buttons
  static unsigned long currentMillis; // variable to store currentMillis
  static int currentThrottle = brakeValue; //current throttle variable

  static unsigned long previousForwardRampMillis = 0; // the last time Forward speed was updated
  static unsigned long previousReverseRampMillis = 0; // the last time Reverse speed was updated
  static unsigned long previousBrakeRampMillis = 0; // will store last time brake throttle was updated

  static boolean cruiseControlOn = 0; // 1 = true, 0 = false
  static unsigned long cruiseControlMillis = 0; // the first time Forward first commanded
  static unsigned long cruiseControlPreviousMillis = 0; // the last time Cruise Control speed was updated
  static unsigned long cruiseControlWaitMs = cruiseControlWaitSec * 1000; // convert Seconds to Ms
  static unsigned long cruiseControlForwardInputDisableDelaySec = cruiseControlForwardInputDisableDelaySec * 1000; // convert Seconds to Ms
  // End Variable Declarations
  
  currentMillis = millis(); // Update currentMillis every loop
  
  getManualInput(motionCommanded); // Get the Manual Control input states (e.g. forward/reverse/brake)

  // Calculate if CruiseControl should be on
  if (cruiseControlEnable) {
    calcCruiseControl(motionCommanded, cruiseControlOn, currentMillis, cruiseControlMillis, previousForwardRampMillis, 
        cruiseControlWaitMs, cruiseControlForwardInputDisableDelaySec);
  }

  // Calculate motion from button states and other details
  calcMotion(motionCommanded, cruiseControlOn, currentThrottle, currentMillis, 
      maxForward, previousForwardRampMillis, forwardRampInterval,
      maxReverse, previousReverseRampMillis, reverseRampInterval,
      maxCruiseControl, cruiseControlPreviousMillis, cruiseControlRampInterval, cruiseControlMillis,
      brakeValue, previousBrakeRampMillis, brakeRampInterval);
  
}

// Return Manual Control input states
void getManualInput(int &motion) {

  boolean inputMan1_value = digitalRead(inputMan1);
  boolean inputMan2_value = digitalRead(inputMan2);
  boolean inputMan3_value = digitalRead(inputMan3);
  boolean inputMan4_value = digitalRead(inputMan4);
  boolean inputMan5_value = digitalRead(inputMan5);
  boolean inputMan6_value = digitalRead(inputMan6);
  boolean inputMan7_value = digitalRead(inputMan7);
  boolean inputMan8_value = digitalRead(inputMan8);
  boolean inputMan9_value = digitalRead(inputMan9);

  // Brake
  if (inputMan1_value == HIGH && inputMan2_value == HIGH) {
    motion = 0;
  } 
  // Forward
  else if (inputMan1_value == LOW && inputMan2_value == HIGH) {
    motion = 1;
  } 
  // Reverse - also disables cruise control
  else if (inputMan1_value == LOW && inputMan2_value == LOW) {
    motion = 2;
  }
  // Cruise Control Disable
  else if (inputMan1_value == HIGH && inputMan2_value == LOW) {
    motion = 3;
  } 
  // If all else fails, then brake
  else {
    motion = 0;
  }
}

void calcCruiseControl(int motionCommanded, boolean &cruiseControlOn, unsigned long currentMillis, unsigned long previousForwardRampMillis, 
          unsigned long &cruiseControlMillis, unsigned long cruiseControlWaitMs, unsigned int cruiseControlRightButtonDisableDelayMs) {

  // Check to see if Forward has been commanded for more than CruiseControlWaitMs
  if (motionCommanded == 1 && !cruiseControlOn && (cruiseControlMillis != 0 && (currentMillis - cruiseControlMillis) >= cruiseControlWaitMs)) {
    cruiseControlOn = 1;
    
    #if defined(Debug)
      Serial.print("CRUISE CONTROL ENABLED");Serial.print("   ");
    #endif
  }
  // Disable Cruise Control if Brake or Reverse is commanded
  else if (cruiseControlOn && (motionCommanded == 2 || motionCommanded == 3)) {
    cruiseControlOn = 0;
    cruiseControlMillis = 0;
    
    #if defined(Debug)
      Serial.print("CRUISE CONTROL DISABLED");Serial.print("   ");
    #endif
  }
  // Disable Cruise Control if cruiseControlOn, Forward commanded and cruiseControlRightButtonDisableDelayMs has elapsed
  else if (cruiseControlOn && motionCommanded == 1 && currentMillis - (cruiseControlMillis + cruiseControlWaitMs) >= cruiseControlRightButtonDisableDelayMs) {
    cruiseControlOn = 0;
    cruiseControlMillis = 0;
    
    #if defined(Debug)
      Serial.print("CRUISE CONTROL DISABLED");Serial.print("   ");
    #endif
  }

}

void calcMotion(int motionCommanded, boolean &cruiseControlOn, int &currentThrottle, unsigned long &currentMillis, 
      int maxForward, unsigned long &previousForwardRampMillis, int forwardRampInterval,
      int maxReverse, unsigned long &previousReverseRampMillis, int reverseRampInterval,
      int maxCruiseControlForward, unsigned long &previousCruiseControlRampMillis, int cruiseControlForwardRampInterval, unsigned long &cruiseControlMillis,
      int brakeValue, unsigned long &previousBrakeRampMillis, int brakeRampInterval) {

  //int throttle = currentThrottle;

  // Forward commanded and reverseMotorDirection = false PLUS
  // cruiseControlOn = false -> forward is different to CruiseControl 
  // currentThrottle >= brakeValue -> when going from reverse to forward, brakeRampInterval unsed until currentThrottle = brakeValue 
  // currentThrottle <= maxForward -> if forward commanded BEFORE throttle is below maxForward then vehicle must be braked first
  if (motionCommanded == 1 && !cruiseControlOn && !reverseMotorDirection && currentThrottle >= brakeValue && currentThrottle <= maxForward + 1) {
    #if defined(Debug)
    Serial.print("Forward - ");
    #endif
    increaseThrottle(currentMillis, previousForwardRampMillis, forwardRampInterval, currentThrottle, maxForward);
    // If the first time Forward commanded since last CruiseControlMillis reset update CruiseControlMillis
    if (cruiseControlMillis == 0) {
      cruiseControlMillis = millis();
      
      #if defined(Debug)
      Serial.print("Cruise Control Millis: ");Serial.print(cruiseControlMillis);Serial.print("   ");
    #endif
    }
  }
  // Forward commanded and reverseMotorDirection = true PLUS
  // cruiseControlOn = false -> forward is different to CruiseControl 
  // currentThrottle >= brakeValue -> when going from reverse to forward, brakeRampInterval unsed until currentThrottle = brakeValue 
  // currentThrottle <= maxForward -> if forward commanded BEFORE throttle is below maxForward then vehicle must be braked first
  else if (motionCommanded == 1 && !cruiseControlOn && reverseMotorDirection && currentThrottle <= brakeValue && currentThrottle >= maxForward - 1) {
    #if defined(Debug)
    Serial.print("Forward - ");
    #endif
    reduceThrottle(currentMillis, previousForwardRampMillis, forwardRampInterval, currentThrottle, maxForward);
  }
  // Reverse commanded and reverseMotorDirection = false
  else if (motionCommanded == 2  && !reverseMotorDirection && currentThrottle <= brakeValue) {
    #if defined(Debug)
    Serial.print("Reverse - ");
    #endif
    reduceThrottle(currentMillis, previousReverseRampMillis, reverseRampInterval, currentThrottle, maxReverse);
    cruiseControlMillis = 0; // reset CruiseControlMillis counter
  }
  // Reverse commanded and reverseMotorDirection = true  
  else if (motionCommanded == 2  && reverseMotorDirection && currentThrottle >= brakeValue) {
    #if defined(Debug)
    Serial.print("Reverse - ");
    #endif
    increaseThrottle(currentMillis, previousReverseRampMillis, reverseRampInterval, currentThrottle, maxReverse);
    cruiseControlMillis = 0; // reset CruiseControlMillis counter
  }
  // cruiseControlOn = true and reverseMotorDirection = false
  else if (cruiseControlOn && !reverseMotorDirection) {
    #if defined(Debug)
    Serial.print("Cruise Control - ");
    #endif
    increaseThrottle(currentMillis, previousCruiseControlRampMillis, cruiseControlForwardRampInterval, currentThrottle, maxCruiseControlForward);
  }
  // cruiseControlOn = true and reverseMotorDirection = true
  else if (cruiseControlOn && reverseMotorDirection) {
    #if defined(Debug)
    Serial.print("Cruise Control - ");
    #endif
    reduceThrottle(currentMillis, previousCruiseControlRampMillis, cruiseControlForwardRampInterval, currentThrottle, maxCruiseControlForward);
  }
  // Brake from Positive Throttle
  else if (!cruiseControlOn && currentThrottle > brakeValue) {
    #if defined(Debug)
    Serial.print("Brake FP - ");
    #endif
    reduceThrottle(currentMillis, previousBrakeRampMillis, brakeRampInterval, currentThrottle, brakeValue);
    cruiseControlMillis = 0; // reset CruiseControlMillis counter
  }
  // Brake from Negative Throttle
  else if (!cruiseControlOn && currentThrottle < brakeValue) {
    #if defined(Debug)
    Serial.print("Brake FN - ");
    #endif
    increaseThrottle(currentMillis, previousBrakeRampMillis, brakeRampInterval, currentThrottle, brakeValue);
    cruiseControlMillis = 0; // reset CruiseControlMillis counter
  }
  // Brake
  else if ((motionCommanded == 0 || motionCommanded == 3) && currentThrottle == brakeValue) {
    //sendThrottleCommand(brakeValue); // Send BrakeValue to controller anyway as a keepalive for controllers that need it
    cruiseControlMillis = 0;  // reset CruiseControlMillis counter
    
    #if defined(Debug)
    Serial.print("Brake - ");Serial.print("throttle: ");Serial.print(currentThrottle);Serial.println();
    #endif
  }
  
}

void increaseThrottle(unsigned long currentMillis, unsigned long &previousMillis, unsigned int interval, int &currentThrottle, int throttleTarget) {

  if (currentMillis - previousMillis >= interval && currentThrottle <= throttleTarget) {
      currentThrottle++;
      previousMillis = currentMillis;
  }
  // Send throttle command even if there is no update -> in essence a keepalive for the motor controller
  sendThrottleCommand(currentThrottle);
  
  #if defined(Debug)
    Serial.print("throttle: ");Serial.print(currentThrottle);Serial.println();
  #endif

}

void reduceThrottle(unsigned long currentMillis, unsigned long &previousMillis, unsigned int interval, int &currentThrottle, int throttleTarget) {

  if (currentMillis - previousMillis >= interval && currentThrottle >= throttleTarget) {
      currentThrottle--;
      previousMillis = currentMillis;
  }
  // Send throttle command even if there is no update -> in essence a keepalive for the motor controller
  sendThrottleCommand(currentThrottle);
  
  #if defined(Debug)
    Serial.print("throttle: ");Serial.print(currentThrottle);Serial.println();
  #endif

}

// Send Throttle Command to the motor controller
void sendThrottleCommand(int currentThrottle) {

#if defined(MC_MCP4XXX)
  MCP4XXX(currentThrottle);
#endif

#if defined(MC_SYREN50)
  Syren50Throttle(currentThrottle);
#endif

#if defined(MC_SABER2X32)
  Sabertooth2x32Throttle(currentThrottle);
#endif
}

#if defined(MC_MCP4XXX)
// Code to send potentiometer value to MCPXXX family of digital potentiometers via SPI
void MCP4XXXWrite(signed int currentThrottle) {
  digitalWrite(MCP4XXX_CS, LOW); // Select MCP4XXX
  SPI.transfer(address);
  SPI.transfer(currentThrottle); // Send throttle value
  digitalWrite(MCP4XXX_CS, HIGH); // Deselect MCP4XXX
}
#endif

#if defined(MC_SYREN50)
// Code to control a Dimension Engineering motor controller
void Syren50Throttle(signed int currentThrottle) {

  ST.motor(1, currentThrottle);
  
}
#endif

#if defined(MC_SABER2X32)
// Code to control a Dimension Engineering motor controller
void Sabertooth2x32Throttle(signed int currentThrottle) {

  ST.motor(1, currentThrottle);
  ST.motor(2, -currentThrottle);
  
}
#endif

