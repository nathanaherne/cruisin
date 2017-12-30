#include "settings.h"

void setup() {

  // Calculate variables
  calculateVariables();

  // Setup the input pins for the buttons
  pinMode(rightButton, INPUT_PULLUP);
  pinMode(leftButton, INPUT_PULLUP);

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
  
  // Stop motor(s)
  sendThrottleCommand();

  // Disable Cruise Control
  cruiseControlMillis = millis();

  #ifdef Debug
    // Configure serial port to send debugging information
    Serial.begin(9600);
    // Print some information to the Serial Port
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

void calculateVariables(){
  
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
  forwardRampInterval = (forwardRampPercent * 100) / 100; //milliseconds between Forward updates
  reverseRampInterval = (reverseRampPercent * 100) / 100; //milliseconds between Reverse updates
  brakeRampInterval =  (brakeRampPercent * 100) / 100; //milliseconds between Reverse updates
  cruiseControlForwardRampInterval = (cruiseControlForwardRampPercent * 100) / 100; //milliseconds between Reverse updates
  
}

void loop() {

  // Variable to store motion commands from momentary buttons
  static int motion;

  // Update currentMillis every loop
   currentMillis = millis();
  
  // Get the type of motion commanded (e.g. forward/reverse/brake)
  motion = getMotion();

  // If Cruise Control has been enabled, then calculate if it should be on
  if (enableCruiseControl) {
    calcCruiseControl(motion);
  }

  // Calculate motion from button states and other details
  doMotion(motion);
  
}

// Calculates action based on button states
int getMotion() {

  boolean rightButtonValue = digitalRead(rightButton);
  boolean leftButtonValue = digitalRead(leftButton);

  // Forward
  if (rightButtonValue == LOW && leftButtonValue == HIGH) {
    return 1;
  } 
  // Reverse - also disables cruise control
  else if (rightButtonValue == LOW && leftButtonValue == LOW) {
    return 2;
  }
  // Cruise Control Disable
  else if (rightButtonValue == HIGH && leftButtonValue == LOW) {
    return 3;
  }  
  // Brake
  else {
    return 0;
  }
}

void doMotion(int motion) {

  // Forward commanded, cruiseControlOn = false and reverseMotorDirection = false
  if (motion == 1 && !cruiseControlOn && !reverseMotorDirection && currentThrottle >= brakeValue) {
    
    forward();
  }
  // Forward commanded, cruiseControlOn = false and reverseMotorDirection = true
  else if (motion == 1 && !cruiseControlOn && reverseMotorDirection && currentThrottle <= brakeValue) {
    
    forwardInvert();
  }  
  // Reverse and reverseMotorDirection = false
  else if (motion == 2  && !reverseMotorDirection && currentThrottle <= brakeValue) {
    
    reverse();
    cruiseControlMillis = currentMillis;
  }
  // Reverse and reverseMotorDirection = true  
  else if (motion == 2  && reverseMotorDirection && currentThrottle >= brakeValue) {
    
    reverseInvert();
    cruiseControlMillis = currentMillis;
  }
  // Cruise Control and reverseMotorDirection = false
  else if (cruiseControlOn && !reverseMotorDirection) {
  
    cruiseForward();
  }
  // Cruise Control and reverseMotorDirection = true
  else if (cruiseControlOn && reverseMotorDirection) {

    cruiseForwardInvert();
  }
  // Brake from Positive Throttle
  else if (!cruiseControlOn && currentThrottle > brakeValue) {
    brakeFromPositiveThrottle();
    cruiseControlMillis = currentMillis;
  }
  // Brake from Negative Throttle
  else if (!cruiseControlOn && currentThrottle < brakeValue) {
    brakeFromNegativeThrottle();
    cruiseControlMillis = currentMillis;
  }
  // Brake
  else if ((motion == 0 || motion == 3) && currentThrottle == brakeValue) {
    #ifdef Debug
    Serial.print("Brake - ");Serial.print("throttle: ");Serial.print(currentThrottle);Serial.println();
    #endif
    cruiseControlMillis = currentMillis;
  }
  
}

void calcCruiseControl(int motion) {

  // Check to see if Forward has been commanded for more than CruiseControlInterval
  if (motion == 1 && !cruiseControlOn && currentMillis - cruiseControlMillis >= cruiseControlForwardWait) {
    cruiseControlOn = 1;
    cruiseControlForwardDelay = currentMillis;
    
    #ifdef Debug
      Serial.print("CRUISE CONTROL ENABLED");Serial.println();
    #endif
  }

  // Disable Cruise Control if actioned to be disabled
  if (cruiseControlOn && (motion == 2 || motion == 3)) {
    cruiseControlOn = 0;
    
    #ifdef Debug
      Serial.print("CRUISE CONTROL DISABLED");Serial.println();
    #endif
  }
  
}

void forward() {

  if (currentMillis - previousForwardRampMillis >= forwardRampInterval) {
    for (currentThrottle; currentThrottle <= maxForward;) {
      currentThrottle++;
      previousForwardRampMillis = currentMillis;
      break;
      }
  }
  // Send throttle command even if there is no update -> in essence a keepalive for the motor controller
  sendThrottleCommand();
  
  #ifdef Debug
    Serial.print("Forward - ");Serial.print("throttle:  ");Serial.print(currentThrottle);Serial.println();  
  #endif

}

void forwardInvert() {

  if (currentMillis - previousForwardRampMillis >= forwardRampInterval) {
    for (currentThrottle; currentThrottle >= maxForward;) {
      currentThrottle--;
      previousForwardRampMillis = currentMillis;
      break;
      }
  }
  // Send throttle command even if there is no update -> in essence a keepalive for the motor controller
  sendThrottleCommand();
  
  #ifdef Debug
    Serial.print("Forward - ");Serial.print("throttle:  ");Serial.print(currentThrottle);Serial.println();  
  #endif

}

void reverse() {

  if (currentMillis - previousReverseRampMillis >= reverseRampInterval) {
    for (currentThrottle; currentThrottle >= maxReverse;) {
      currentThrottle--;
      previousReverseRampMillis = currentMillis;
      break;
      }
  }
  // Send throttle command even if there is no update -> in essence a keepalive for the motor controller
  sendThrottleCommand();
  
  #ifdef Debug
    Serial.print("Reverse - ");Serial.print("throttle:  ");Serial.print(currentThrottle);Serial.println(); 
  #endif
}

void reverseInvert() {

  if (currentMillis - previousReverseRampMillis >= reverseRampInterval) {
    for (currentThrottle; currentThrottle <= maxReverse;) {
      currentThrottle++;
      previousReverseRampMillis = currentMillis;
      break;
    }
  }
  // Send throttle command even if there is no update -> in essence a keepalive for the motor controller
  sendThrottleCommand();
  
  #ifdef Debug
    Serial.print("Reverse - ");Serial.print("throttle:  ");Serial.print(currentThrottle);Serial.println(); 
  #endif
}

void brakeFromPositiveThrottle() {

  if (currentMillis - previousBrakeRampMillis >= brakeRampInterval) {  
    for (currentThrottle; currentThrottle >= brakeValue;) {
      currentThrottle--;
      previousBrakeRampMillis = currentMillis;
      break;
    }
  }
  // Send throttle command even if there is no update -> in essence a keepalive for the motor controller
  sendThrottleCommand();
  
  #ifdef Debug
    Serial.print("Brake FP - "); Serial.print("throttle: ");Serial.print(currentThrottle);Serial.println();
  #endif
}

void brakeFromNegativeThrottle() {

  if (currentMillis - previousBrakeRampMillis >= brakeRampInterval) {  
    for (currentThrottle; currentThrottle <= brakeValue;) {
      currentThrottle++;
      previousBrakeRampMillis = currentMillis;
      break;
    }
  }
  // Send throttle command even if there is no update -> in essence a keepalive for the motor controller
  sendThrottleCommand();
  
  #ifdef Debug
    Serial.print("Brake FN - "); Serial.print("throttle: ");Serial.print(currentThrottle);Serial.println();
  #endif
}

void cruiseForward() {

  if (currentMillis - previousCruiseControlRampMillis >= cruiseControlForwardRampInterval) {
    for (currentThrottle; currentThrottle <= maxCruiseControlForward;) {
      currentThrottle++;
      previousCruiseControlRampMillis = currentMillis;
      break;
    }
  }
  // Send throttle command even if there is no update -> in essence a keepalive for the motor controller
  sendThrottleCommand();
  
  #ifdef Debug
  Serial.print("Cruise Control - ");Serial.print("throttle:  ");Serial.print(currentThrottle);Serial.println();
  #endif
}

void cruiseForwardInvert() {

  if (currentMillis - previousCruiseControlRampMillis >= cruiseControlForwardRampInterval) {
    for (currentThrottle; currentThrottle <= maxCruiseControlForward;) {
      currentThrottle++;
      previousCruiseControlRampMillis = currentMillis;
      break;
      }
  }
  // Send throttle command even if there is no update -> in essence a keepalive for the motor controller
  sendThrottleCommand();
  
  #ifdef Debug
  Serial.print("Cruise Control - ");Serial.print("throttle:  ");Serial.print(currentThrottle);Serial.println();
  #endif
}

// Send Throttle Command to the motor controller
void sendThrottleCommand() {

#if defined(SpiPot5v)
  SpiPot5vWrite(currentThrottle);
#endif

#if defined(Syren50) || defined(Sabertooth2x32)
  DeControllerThrottle(currentThrottle);
#endif
  
}

#ifdef SpiPot5v
// Code to control a MCP4151 digital potentiometer via SPI
void SpiPot5vWrite(int throttleValue) {
  digitalWrite(CS, LOW);
  SPI.transfer(address);
  SPI.transfer(throttleValue);
  digitalWrite(CS, HIGH);
}
#endif

#ifdef SpiPot12v
// Code to control a MCP4151 digital potentiometer via SPI
void SpiPot12vWrite(int throttleValue) {
  digitalWrite(CS, LOW);
  SPI.transfer(address);
  SPI.transfer(throttleValue);
  digitalWrite(CS, HIGH);
}
#endif

#if defined(Syren50)
// Code to control a Dimension Engineering motor controller
void DeControllerThrottle(int throttleValue){

  ST.motor(1, throttleValue);
  
}
#endif

