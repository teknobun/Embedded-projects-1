#include <Wire.h>              // I2C library, needed for the compass
#include <LiquidCrystal.h>

// Initialized with specific pins: (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

// Motor pins
#define Motor_forward    0
#define Motor_return     1
#define Motor_L_dir_pin  8
#define Motor_R_dir_pin  7
#define Motor_L_pwm_pin  10
#define Motor_R_pwm_pin  9

// Encoder settings
const byte ENC_L_PIN = 2;
const byte ENC_R_PIN = 3;
const float PULSES_PER_CM = 13.5;
const int DRIVE_PWM = 200;          // driving speed (0-255)

// Compass settings
const byte CMPS14_ADDRESS = 0x60;   // I2C address of the compass
const byte REG_HEADING_8  = 0x01;   // register with the 8-bit heading (0-255)

// Turning settings
const int offset = 4;               // allowed heading error: +/- 4 degrees
const int TURN_PWM = 255;           // spin speed while turning
const int LOOP_DELAY_MS = 30;       // pause between compass checks while turning
const unsigned long TURN_TIMEOUT_MS = 10000; // maximum duration of one turn

// Pulse counters, changed inside interrupts so they must be volatile
volatile unsigned long encLeftCount  = 0;
volatile unsigned long encRightCount = 0;
void encLeftISR()  { encLeftCount++; }
void encRightISR() { encRightCount++; }

void setup(){
  Serial.begin(9600);
 
  lcd.begin(20, 4);     // Set for 20 columns and 4 rows
  Wire.begin();         // start I2C for the compass

  // Motor pins are outputs, encoder pins trigger an interrupt on every pulse
  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);
  pinMode(ENC_L_PIN, INPUT_PULLUP);
  pinMode(ENC_R_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_L_PIN), encLeftISR, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_R_PIN), encRightISR, RISING);

  // Optional boot message on the screen
  lcd.setCursor(0, 0);
  lcd.print("Ready for Serial");

  Serial.println("Write something to the serial monitor.");
  delay(1000);          // Allow serial communication to stabilize at boot
}

void loop(){
  if (Serial.available() > 0){
    String message = Serial.readStringUntil('\n');
    Serial.print("Message received, content: ");
    Serial.println(message);

    // Commands: LCD, Move, Turn (relative), and TurnTo (absolute).
    int pos_lcd = message.indexOf("LCD");
    if (pos_lcd > -1) {
      Serial.println("Command = LCD");
      int pos_colon = message.indexOf(":");

      if (pos_colon > -1) {
        String textToPrint = message.substring(pos_colon + 1);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(textToPrint);
        Serial.print("Printed to LCD: ");
        Serial.println(textToPrint);
      }
    }
    // drive command, e.g. Move:10 drives forward, Move:-10 drives backward
    else if (message.indexOf("Move") > -1) {
      Serial.println("Command = Move");
      int pos_colon = message.indexOf(":");

      if (pos_colon > -1) {
        int value = message.substring(pos_colon + 1).toInt();

        lcd.clear();                    // print the entire command on the LCD
        lcd.setCursor(0, 0);
        lcd.print(message);

        Serial.print("Driving cm: ");
        Serial.println(value);
        drive(value);                   // positive = forward, negative = backward
      }
    }
    // Absolute heading, e.g. TurnTo:0 faces north by the shortest route.
    else if (message.startsWith("TurnTo:")) {
      int value = message.substring(7).toInt();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(message);

      Serial.print("Turning to heading: ");
      Serial.println(value);
      turnToHeading(value);
    }
    // Relative rotation: positive is clockwise, negative is counterclockwise.
    else if (message.startsWith("Turn:")) {
      int value = message.substring(5).toInt();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(message);

      Serial.print("Turning by degrees: ");
      Serial.println(value);
      turnByDegrees(value);
    }
    else {
      Serial.println("No recognized command found, try LCD:Hello, Move:10, Turn:90 or TurnTo:0\n");
    }
  }
}

// Drives the given number of cm using encoder pulses
// positive value = forward, negative value = backward
void drive(int cm) {
  unsigned long targetPulses = (unsigned long)(abs(cm) * PULSES_PER_CM);
  byte dir = (cm >= 0) ? Motor_forward : Motor_return;   // negative = backward

  // Reset both pulse counters (interrupts paused while writing the 4-byte values)
  noInterrupts();
  encLeftCount = 0;
  encRightCount = 0;
  interrupts();

  digitalWrite(Motor_L_dir_pin, dir);
  digitalWrite(Motor_R_dir_pin, dir);
  analogWrite(Motor_L_pwm_pin, DRIVE_PWM);
  analogWrite(Motor_R_pwm_pin, DRIVE_PWM);

  // Drive until the average pulse count reaches the target
  // (10 s timeout so the car stops if an encoder is not counting or the car is stuck)
  unsigned long start = millis();
  while (millis() - start < 10000) {
    noInterrupts();
    unsigned long pulses = (encLeftCount + encRightCount) / 2;
    interrupts();
    if (pulses >= targetPulses) break;
  }

  analogWrite(Motor_L_pwm_pin, 0);   // stop the motors
  analogWrite(Motor_R_pwm_pin, 0);
}

// Turns through a signed angle, including full revolutions.
// Accumulating small heading changes preserves rotation across 0/360 degrees.
void turnByDegrees(int degrees) {
  stopMotors();

  const long requestedRotation = abs((long)degrees);
  if (requestedRotation <= offset) {
    return;
  }

  int previous = readCompass();
  if (previous == -1) {
    Serial.println("Compass read failed.");
    return;
  }

  const int direction = (degrees > 0) ? 1 : -1;
  long rotated = 0;
  unsigned long start = millis();

  while (rotated < requestedRotation - offset) {
    if (millis() - start >= TURN_TIMEOUT_MS) {
      stopMotors();
      Serial.println("Relative turn timed out.");
      return;
    }

    if (direction > 0) {
      turnRight();
    } else {
      turnLeft();
    }
    delay(LOOP_DELAY_MS);

    int current = readCompass();
    if (current == -1) {
      stopMotors();
      Serial.println("Compass read failed.");
      return;
    }

    rotated += direction * angleDiff(current, previous);
    previous = current;
  }
  stopMotors();
}

// Turns the car on the spot until the compass shows the target heading (+/- offset)
void turnToHeading(int target) {
  target %= 360;
  if (target < 0) {
    target += 360;
  }

  unsigned long start = millis();
  while (true) {
    if (millis() - start >= TURN_TIMEOUT_MS) {
      stopMotors();
      Serial.println("Heading turn timed out.");
      return;
    }

    int current = readCompass();
    if (current == -1) break;                       // compass error: stop for safety

    // CHANGED: shortest way to the target instead of comparing the raw numbers
    int diff = angleDiff(target, current);          // positive = target is to the right

    if (abs(diff) <= offset) {
      break;                                        // close enough: done
    }

    if (diff > 0) {
      turnRight();
    } else {
      turnLeft();
    }
    delay(LOOP_DELAY_MS);
  }
  stopMotors();
}

// NEW: shortest way from the current heading to the target, from -180 to +180
// positive = target is to the right (clockwise), negative = target is to the left

int angleDiff(int target, int current) {
  int diff = (target - current) % 360;
  if (diff > 180)  diff -= 360;
  if (diff < -180) diff += 360;
  return diff;
}

// Reads the compass, returns the heading in degrees (0-359), or -1 on error
int readCompass() {
  Wire.beginTransmission((int)CMPS14_ADDRESS);
  Wire.write(REG_HEADING_8);
  if (Wire.endTransmission(false) != 0) {
    return -1;                                      // I2C communication error
  }

  Wire.requestFrom((int)CMPS14_ADDRESS, 1);
  if (Wire.available() >= 1) {
    byte heading8 = Wire.read();                    // 8-bit value, 0-255
    return (int)(heading8 * 360.0 / 256.0);         // 256 steps = 360 degrees
  }
  return -1;                                        // read failure
}

// Spin right on the spot: left wheel forward, right wheel backward
void turnRight() {
  digitalWrite(Motor_L_dir_pin, Motor_forward);
  digitalWrite(Motor_R_dir_pin, Motor_return);
  analogWrite(Motor_L_pwm_pin, TURN_PWM);
  analogWrite(Motor_R_pwm_pin, TURN_PWM);
}

// Spin left on the spot: left wheel backward, right wheel forward
void turnLeft() {
  digitalWrite(Motor_L_dir_pin, Motor_return);
  digitalWrite(Motor_R_dir_pin, Motor_forward);
  analogWrite(Motor_L_pwm_pin, TURN_PWM);
  analogWrite(Motor_R_pwm_pin, TURN_PWM);
}

// Stop both motors
void stopMotors() {
  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);
}
