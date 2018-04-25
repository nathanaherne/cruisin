/**********************************************************************************************************************
MCP4XXX DIGITAL POTENTIOMETER
**********************************************************************************************************************/
#if defined(MCP4XXX)

  // Interface with digital potentiometer via SPI
  #include <SPI.h>
  byte address = 0x00;
  
  // SPI pins configured in pins.h

  // Pot output Values
  const int throttleMin = 1; // Potentiometer minimum or Reverse value
  const int brakeValue = 128; // Potentiometer Brake value
  const int throttleMax = 255; // Potentiometer Max or Forward value

#endif
/**********************************************************************************************************************/

/**********************************************************************************************************************
DIMENSION ENGINEERING SYREN50 & SABERTOOTH2x32 MOTOR CONTROLLERS
**********************************************************************************************************************/
#if defined(MC_SYREN50) || defined(MC_SABER2X32)

  #include <Sabertooth.h>
  #include <SoftwareSerial.h>
  
  // Throttle Values for Syren50 and Sabertooth2x32
  const int throttleMin = -2047;  // Throttle minimum or Reverse value
  const int brakeValue = 0; // Syren50 Brake value
  const int throttleMax = 2047; // Throttle maximum or Forward value
  
  SoftwareSerial SabertoothSWSerial(SSRxPin, SSTxPin); // RX on no pin (unused), TX to S1 of Syren
  Sabertooth ST(128, SabertoothSWSerial); // Address 128, and use SWSerial as the serial port.
  const int deControllerTimeout = 1000; // 1 second between updates or controller shuts down

#endif

/**********************************************************************************************************************
PWM OUTPUT TO MOTOR CONTROLLERS
**********************************************************************************************************************/

  // Throttle values PWM output
  const int throttleMin = 1000;  // PWM minimum or Reverse value
  const int brakeValue = 1500; // PWM brake value
  const int throttleMax = 2000; // PWM maximum or Forward value

/**********************************************************************************************************************
MPU9250
**********************************************************************************************************************/
#if defined(IMU_MPU9250)

  #include <Wire.h>

  //Setup MPU9250

#endif

/**********************************************************************************************************************
CONFIGURATION FOR I2C OLED
**********************************************************************************************************************/
#if defined(OLED_128X64)

  #include <i2c_t3.h>

  //Setup MPU9250

#endif
