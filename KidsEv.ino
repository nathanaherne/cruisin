#include "settings.h"

void setup() {

  #ifdef SpiPot5v
    pinMode (CS, OUTPUT);
    SPI.begin();
  #endif

  #ifdef Syren50
    pinMode(SwSerialTxPin, OUTPUT);
    SabertoothSWSerial.begin(9600);
    ST.setTimeout(deControllerTimeout); // Controller must recieve commands every X milliseconds or it will stop motors
    ST.setRamping(0);
    ST.autobaud();
  #endif

  // Calculate variables
  calculateGlobalVariables();

  // Setup the input pins for the buttons
  pinMode(rightButton, INPUT_PULLUP);
  pinMode(leftButton, INPUT_PULLUP);
  
  // Stop motor(s)
  sendThrottleCommand(brakeValue);

  #ifdef Debug
    // Configure serial port to send debugging information
    Serial.begin(9600);
    // Print some information to the Serial Port
    Serial.println();
    Serial.print("******************************************************************************************");Serial.println();
      if (reverseMotorDirection) {
        Serial.print("Motor Direction Reversed: ");Serial.print("True");Serial.println();
      } else {
        Serial.print("Motor Direction Reversed: ");Serial.print("False");Serial.println();
      }
      if (enableCruiseControl) {
        Serial.print("Cruise Control Enabled: ");Serial.print("True");Serial.println();
      } else {
        Serial.print("Cruise Control Enabled: ");Serial.print("False");Serial.println();
      }
    Serial.print("--------------------------------------------");Serial.println();
    Serial.print("Maximum Forward Throttle Percent: ");Serial.print(maxForwardPercent);Serial.print("%");Serial.println();
    Serial.print("Maximum Forward Throttle Output: ");Serial.print(maxForward);Serial.println();
    Serial.print("Forward Ramp Percent: ");Serial.print(forwardRampPercent);Serial.print("%");Serial.println();
    Serial.print("Forward Ramp Inverval: ");Serial.print(forwardRampInterval);Serial.print(" milliseconds");Serial.println();
    Serial.print("--------------------------------------------");Serial.println();
    Serial.print("Maximum Reverse Throttle Percent: ");Serial.print(maxReversePercent);Serial.print("%");Serial.println();
    Serial.print("Maximum Reverse Throttle Output: ");Serial.print(maxReverse);Serial.println();
    Serial.print("Reverse Ramp Percent: ");Serial.print(reverseRampPercent);Serial.print("%");Serial.println();
    Serial.print("Reverse Ramp Inverval: ");Serial.print(reverseRampInterval);Serial.print(" milliseconds");Serial.println();
    Serial.print("--------------------------------------------");Serial.println();
    Serial.print("Maximum Cruise Control Throttle Percent: ");Serial.print(maxCruiseControlForwardPercent);Serial.print("%");Serial.println();
    Serial.print("Maximum Cruise Control Throttle Output: ");Serial.print(maxCruiseControlForward);Serial.println();
    Serial.print("Cruise Control Ramp Percent: ");Serial.print(cruiseControlForwardRampPercent);Serial.print("%");Serial.println();
    Serial.print("CruiseControl Ramp Inverval: ");Serial.print(cruiseControlForwardRampInterval);Serial.print(" milliseconds");Serial.println();
    Serial.print("--------------------------------------------");Serial.println();
    Serial.print("Brake Value: ");Serial.print(brakeValue);Serial.println();
    Serial.print("Brake Ramp Percent: ");Serial.print(brakeRampPercent);Serial.print("%");Serial.println();
    Serial.print("Brake Ramp Interval: ");Serial.print(brakeRampInterval);Serial.print(" milliseconds");Serial.println();
    Serial.print("******************************************************************************************");Serial.println();
    delay(500);
  #endif
  
}

void calculateGlobalVariables(){
  
  // Calculations for maxForward
  if (reverseMotorDirection) {
    maxForward = brakeValue - (((brakeValue - throttleMin) * maxForwardPercent) / 100); // Based on maxForwardPercent & Throttle Output Value settings
  } else if (!reverseMotorDirection) {
    maxForward = brakeValue + (((throttleMax - brakeValue) * maxForwardPercent) / 100); // Based on maxForwardPercent & Throttle Output Value settings
  } else {
    maxForward = brakeValue;
  }
  
  // Calculation for maxReverse
  if (reverseMotorDirection) {
    maxReverse = brakeValue + (((throttleMax - brakeValue) * maxReversePercent) / 100); // Based on maxReversePercent & Throttle Output Value settings
  } else if (!reverseMotorDirection) {
    maxReverse = brakeValue - (((brakeValue - throttleMin) * maxReversePercent) / 100); // Based on maxReversePercent & Throttle Output Value settings
  } else {
    maxReverse = brakeValue;
  }
  
  // Calculation for maxCCForward
  if (reverseMotorDirection) {
    maxCruiseControlForward = brakeValue - (((brakeValue - throttleMin) * maxCruiseControlForwardPercent) / 100); //Based on maxCruiseControlForwardPercent & Throttle Output Value settings
  } else if (!reverseMotorDirection) {
    maxCruiseControlForward = brakeValue + (((throttleMax - brakeValue) * maxCruiseControlForwardPercent) / 100); // Based on maxCruiseControlForwardPercent & Throttle Output Value settings
  } else {
    maxCruiseControlForward = brakeValue;
  }

  // Calculations for ramp intervals
  forwardRampInterval = (forwardRampPercent * 100) / 100; // milliseconds between Forward updates
  reverseRampInterval = (reverseRampPercent * 100) / 100; // milliseconds between Reverse updates
  brakeRampInterval =  (brakeRampPercent * 100) / 100; // milliseconds between Reverse updates
  cruiseControlForwardRampInterval = (cruiseControlForwardRampPercent * 100) / 100; // milliseconds between Reverse updates
  
}

void loop() {

  // Variable Declarations
  static int motionCommanded; // variable to store motion commands from momentary buttons
  static int motionCalculated; // variable to store motion calculated
  static unsigned long currentMillis; // variable to store currentMillis
  static int currentThrottle = brakeValue; //current throttle variable

  static unsigned long previousForwardRampMillis = 0; // the last time Forward speed was updated
  static unsigned long previousReverseRampMillis = 0; // the last time Reverse speed was updated
  static unsigned long previousBrakeRampMillis = 0; // will store last time brake throttle was updated

  static boolean cruiseControlOn = 0; // 1 = true, 0 = false
  static unsigned long cruiseControlMillis = 0; // the first time Forward first commanded
  static unsigned long previousCruiseControlRampMillis = 0; // the last time Cruise Control speed was updated
  static int cruiseControlForwardWaitMs = cruiseControlForwardWaitSec * 1000; // convert Seconds to Ms
  static int cruiseControlRightButtonDisableDelayMs = cruiseControlRightButtonDisableDelaySec * 1000; // convert Seconds to Ms
  // End Variable Declarations
  
  currentMillis = millis(); // Update currentMillis every loop
  
  // Get the button combinations pressed (e.g. forward/reverse/brake)
  motionCommanded = getButtons();

  // If Cruise Control has been enabled, then calculate if it should be on
  if (enableCruiseControl) {
    cruiseControlOn = calcCruiseControl(motionCommanded, currentMillis, cruiseControlMillis, cruiseControlForwardWaitMs, cruiseControlOn, cruiseControlRightButtonDisableDelayMs);
  }

  // Calculate motion from button states and other details
  calcMotion(motionCommanded, cruiseControlOn, currentThrottle, currentMillis, 
      maxForward, previousForwardRampMillis, forwardRampInterval,
      maxReverse, previousReverseRampMillis, reverseRampInterval,
      maxCruiseControlForward, previousCruiseControlRampMillis, cruiseControlForwardRampInterval, cruiseControlMillis,
      brakeValue, previousBrakeRampMillis, brakeRampInterval);
  
}

// Calculates action based on button states
int getButtons() {

  boolean rightButtonValue = digitalRead(rightButton);
  boolean leftButtonValue = digitalRead(leftButton);

  // Forward
  if (rightButtonValue == LOW && leftButtonValue == HIGH) {
    //Serial.print("Forward");Serial.println();
    return 1;
  } 
  // Reverse - also disables cruise control
  else if (rightButtonValue == LOW && leftButtonValue == LOW) {
    //Serial.print("Reverse");Serial.println();
    return 2;
  }
  // Cruise Control Disable
  else if (rightButtonValue == HIGH && leftButtonValue == LOW) {
    //Serial.print("Cruise Control Disable");Serial.println();
    return 3;
  }  
  // Brake
  else {
    //Serial.print("Brake");Serial.println();
    return 0;
  }
}

boolean calcCruiseControl(int motionCommanded, unsigned long currentMillis, unsigned long cruiseControlMillis, int cruiseControlForwardWaitMs, 
          boolean cruiseControlOn, int cruiseControlRightButtonDisableDelayMs) {

  // Check to see if Forward has been commanded for more than CruiseControlForwardInterval
  if (motionCommanded == 1 && !cruiseControlOn && currentMillis - cruiseControlMillis >= cruiseControlForwardWaitMs) {
    cruiseControlOn = 1;
    cruiseControlMillis = currentMillis;
    
    #ifdef Debug
      Serial.print("CRUISE CONTROL ENABLED");Serial.println();
    #endif
  }

  // Disable Cruise Control if actioned to be disabled OR cruiseControlRightButtonDisableDelayMs has elapsed
  if (cruiseControlOn && (motionCommanded == 2 || motionCommanded == 3)) { // || (motionCommanded == 1 && cruiseControlMillis + cruiseControlRightButtonDisableDelayMs > currentMillis)
    cruiseControlOn = 0;
    
    #ifdef Debug
      Serial.print("CRUISE CONTROL DISABLED");Serial.println();
    #endif
  }

  return cruiseControlOn;
  
}

void calcMotion(int motionCommanded, boolean &cruiseControlOn, int &currentThrottle, unsigned long &currentMillis, 
      int maxForward, unsigned long &previousForwardRampMillis, int forwardRampInterval,
      int maxReverse, unsigned long &previousReverseRampMillis, int reverseRampInterval,
      int maxCruiseControlForward, unsigned long &previousCruiseControlRampMillis, int cruiseControlForwardRampInterval, unsigned long &cruiseControlMillis,
      int brakeValue, unsigned long &previousBrakeRampMillis, int brakeRampInterval) {

  int throttle = currentThrottle;

  // Forward commanded and reverseMotorDirection = false PLUS
  // cruiseControlOn = false -> forward is different to CruiseControl 
  // currentThrottle >= brakeValue -> when going from reverse to forward, brakeRampInterval unsed until currentThrottle = brakeValue 
  // currentThrottle <= maxForward -> if forward commanded BEFORE throttle is below maxForward then vehicle must be braked first
  if (motionCommanded == 1 && !cruiseControlOn && !reverseMotorDirection && currentThrottle >= brakeValue && currentThrottle <= maxForward + 1) {
    #ifdef Debug
    Serial.print("Forward - ");
    #endif
    increaseThrottle(currentMillis, previousForwardRampMillis, forwardRampInterval, currentThrottle, maxForward);
  }
  // Forward commanded and reverseMotorDirection = true PLUS
  // cruiseControlOn = false -> forward is different to CruiseControl 
  // currentThrottle >= brakeValue -> when going from reverse to forward, brakeRampInterval unsed until currentThrottle = brakeValue 
  // currentThrottle <= maxForward -> if forward commanded BEFORE throttle is below maxForward then vehicle must be braked first
  else if (motionCommanded == 1 && !cruiseControlOn && reverseMotorDirection && currentThrottle <= brakeValue && currentThrottle >= maxForward + 1) {
    #ifdef Debug
    Serial.print("Forward - ");
    #endif
    reduceThrottle(currentMillis, previousForwardRampMillis, forwardRampInterval, currentThrottle, maxForward);
  }
  // Reverse commanded and reverseMotorDirection = false
  else if (motionCommanded == 2  && !reverseMotorDirection && currentThrottle <= brakeValue) {
    #ifdef Debug
    Serial.print("Reverse - ");
    #endif
    reduceThrottle(currentMillis, previousReverseRampMillis, reverseRampInterval, currentThrottle, maxReverse);
    cruiseControlMillis = currentMillis; // reset CruiseControlMillis counter
  }
  // Reverse commanded and reverseMotorDirection = true  
  else if (motionCommanded == 2  && reverseMotorDirection && currentThrottle >= brakeValue) {
    #ifdef Debug
    Serial.print("Reverse - ");
    #endif
    increaseThrottle(currentMillis, previousReverseRampMillis, reverseRampInterval, currentThrottle, maxReverse);
    cruiseControlMillis = currentMillis; // reset CruiseControlMillis counter
  }
  // cruiseControlOn = true and reverseMotorDirection = false
  else if (cruiseControlOn && !reverseMotorDirection) {
    #ifdef Debug
    Serial.print("Cruise Control - ");
    #endif
    increaseThrottle(currentMillis, previousCruiseControlRampMillis, cruiseControlForwardRampInterval, currentThrottle, maxCruiseControlForward);
  }
  // cruiseControlOn = true and reverseMotorDirection = true
  else if (cruiseControlOn && reverseMotorDirection) {
    #ifdef Debug
    Serial.print("Cruise Control - ");
    #endif
    reduceThrottle(currentMillis, previousCruiseControlRampMillis, cruiseControlForwardRampInterval, currentThrottle, maxCruiseControlForward);
  }
  // Brake from Positive Throttle
  else if (!cruiseControlOn && currentThrottle > brakeValue) {
    #ifdef Debug
    Serial.print("Brake FP - ");
    #endif
    reduceThrottle(currentMillis, previousBrakeRampMillis, brakeRampInterval, currentThrottle, brakeValue);
    cruiseControlMillis = currentMillis; // reset CruiseControlMillis counter
  }
  // Brake from Negative Throttle
  else if (!cruiseControlOn && currentThrottle < brakeValue) {
    #ifdef Debug
    Serial.print("Brake FN - ");
    #endif
    increaseThrottle(currentMillis, previousBrakeRampMillis, brakeRampInterval, currentThrottle, brakeValue);
    cruiseControlMillis = currentMillis; // reset CruiseControlMillis counter
  }
  // Brake
  else if ((motionCommanded == 0 || motionCommanded == 3) && currentThrottle == brakeValue) {
    sendThrottleCommand(brakeValue); // Send BrakeValue to controller anyway as a keepalive for controllers that need it
    cruiseControlMillis = currentMillis;  // reset CruiseControlMillis counter
    
    #ifdef Debug
    Serial.print("Brake - ");Serial.print("throttle: ");Serial.print(currentThrottle);Serial.println();
    #endif
  }
  
}

void increaseThrottle(unsigned long currentMillis, unsigned long &previousMillis, int interval, int &currentThrottle, int throttleTarget) {

  if (currentMillis - previousMillis >= interval) {
    for (currentThrottle; currentThrottle <= throttleTarget;) {
      currentThrottle++;
      previousMillis = currentMillis;
      break;
      }
  }
  // Send throttle command even if there is no update -> in essence a keepalive for the motor controller
  sendThrottleCommand(currentThrottle);
  
  #ifdef Debug
    Serial.print("throttle: ");Serial.print(currentThrottle);Serial.println();
  #endif

  //return previousMillis;

}

void reduceThrottle(unsigned long currentMillis, unsigned long &previousMillis, int interval, int &currentThrottle, int throttleTarget) {

  if (currentMillis - previousMillis >= interval) {
    for (currentThrottle; currentThrottle >= throttleTarget;) {
      currentThrottle--;
      previousMillis = currentMillis;
      break;
      }
  }
  // Send throttle command even if there is no update -> in essence a keepalive for the motor controller
  sendThrottleCommand(currentThrottle);
  
  #ifdef Debug
    Serial.print("throttle: ");Serial.print(currentThrottle);Serial.println();
  #endif

  //return previousMillis;

}

// Send Throttle Command to the motor controller
void sendThrottleCommand(int currentThrottle) {

#if defined(SpiPot5v)
  SpiPot5vWrite(currentThrottle);
#endif

#if defined(Syren50) || defined(Sabertooth2x32)
  DeControllerThrottle(currentThrottle);
#endif
  
}

#ifdef SpiPot5v
// Code to control a MCP4151 digital potentiometer via SPI
void SpiPot5vWrite(int currentThrottle) {
  digitalWrite(CS, LOW);
  SPI.transfer(address);
  SPI.transfer(currentThrottle);
  digitalWrite(CS, HIGH);
}
#endif

#ifdef SpiPot12v
// Code to control a MCP4151 digital potentiometer via SPI
void SpiPot12vWrite(int currentThrottle) {
  digitalWrite(CS, LOW);
  SPI.transfer(address);
  SPI.transfer(currentThrottle);
  digitalWrite(CS, HIGH);
}
#endif

#if defined(Syren50)
// Code to control a Dimension Engineering motor controller
void DeControllerThrottle(int currentThrottle){

  ST.motor(1, currentThrottle);
  
}
#endif

