#include "AccelStepper.h"
#include "NBHX711.h"

// Pin Definitions
uint8_t PIN_HX711_DATA = 3;
uint8_t PIN_HX711_CLK = 2;
uint8_t PIN_SLIDER = A5;
uint8_t PIN_BUTTON = 6;
uint8_t PIN_STBY = A0;
uint8_t PIN_ENABLE = 13;
uint8_t PIN_ERROR = 5; // unused!
uint8_t PIN_MODE0 = 12;
uint8_t PIN_MODE1 = 11;
uint8_t PIN_MODE2 = 10;
uint8_t PIN_MODE3 = 9;

// Globals
float position_calibration = 1.0;
float scale_calibration = -1.0;

// Create Instances
AccelStepper stepper(AccelStepper::DRIVER, PIN_MODE2, PIN_MODE3);
NBHX711 loadcell(PIN_HX711_DATA, PIN_HX711_CLK, 3, 1);

void setup()
{
  // Start up sensors/controllers/communications
  Serial.begin(115200);
  loadcell.begin();
  loadcell.tare();
  stepper.setMaxSpeed(2000);

  // Set pin modes
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_STBY, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);
  pinMode(PIN_ERROR, INPUT);
  pinMode(PIN_MODE0, OUTPUT);
  pinMode(PIN_MODE1, OUTPUT);
  pinMode(PIN_MODE2, OUTPUT);
  pinMode(PIN_MODE3, OUTPUT);

  // Setup the motor for fixed mode
  digitalWrite(PIN_STBY, LOW);
  digitalWrite(PIN_MODE3, HIGH);
  digitalWrite(PIN_MODE2, LOW);
  digitalWrite(PIN_MODE1, HIGH);
  digitalWrite(PIN_MODE0, LOW);
  digitalWrite(PIN_STBY, HIGH);
  digitalWrite(PIN_ENABLE, LOW);
}

void loop()
{
  // Keep reading the load cell so we have are ready to tare
  if (loadcell.update())
  {
    loadcell.getUnits();
  }
  // Check the button and do some simple debounce to make sure it's really
  // held and ready to run the system
  if (!digitalRead(PIN_BUTTON))
  {
    delay(10);
    if (!digitalRead(PIN_BUTTON))
    {
      runsystem();
    }
  }
}

void runsystem()
{
  // Triggered when the run button is pressed - tares the scale
  // and runs the motor sending time, position, and load data
  // back via serial until the button is released.

  // Tare by hand since the tare function in the library is not reliable
  float load_offset = 0.0;
  for (int i=0; i<10; i++)
  {
    load_offset += loadcell.getUnits() * scale_calibration;
    delay(50);
  }
  load_offset /= 10;
  
  stepper.setCurrentPosition(0);
  digitalWrite(PIN_ENABLE, HIGH);
  delay(10);
  uint32_t start_millis = millis();
  while (1)
  {
    stepper.runSpeed();
    
    // Read the speed adjustment and set the motor
    uint16_t slider = analogRead(PIN_SLIDER);
    float motor_speed = map(slider, 0, 1023, 2000, 0);
    stepper.setSpeed(motor_speed);
    stepper.runSpeed();
    //Serial.println(digitalRead(PIN_ERROR));
    // Get the load cell data and send it back if new data are ready
    
    if (loadcell.update())
    {
      Serial.print(millis() - start_millis);
      Serial.print(",");
      Serial.print(stepper.currentPosition() * position_calibration);
      Serial.print(",");
      Serial.println(loadcell.getUnits() * scale_calibration - load_offset);
    }
    
    // Check if the button is still down or not
    if (digitalRead(PIN_BUTTON))
    {
      stepper.stop();
      digitalWrite(PIN_ENABLE, LOW);
      break;
    }
  }  // while
}  // func
