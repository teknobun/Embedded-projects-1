#include <Wire.h>
#include <LiquidCrystal.h>

// LCD configuration (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

// Compass configuration
const byte CMPS14_ADDRESS = 0x60;
const byte REG1_ADDRESS = 0x01;

// Motor pins and directions
#define Motor_forward    0
#define Motor_return     1
#define Motor_L_dir_pin  8
#define Motor_R_dir_pin  7
#define Motor_L_pwm_pin  10
#define Motor_R_pwm_pin  9

// Target heading and turning settings
const int TARGET_HEADING = 90;
const int OFFSET = 4;

const int TURN_PWM = 200;
const int TURN_PWM_SLOW = 150;
const int SLOW_ZONE = 30;

const unsigned long DISPLAY_MS = 200;

void setup() {
  lcd.begin(20, 4);
  Wire.begin();
  Serial.begin(9600);

  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);

  stopMotors();
}

void loop() {
  find_heading(TARGET_HEADING);
}

bool readCompass(int &degrees) {
  Wire.beginTransmission(CMPS14_ADDRESS);
  Wire.write(REG1_ADDRESS);

  if (Wire.endTransmission() != 0) {
    return false;
  }

  if (Wire.requestFrom(CMPS14_ADDRESS, (byte)1) != 1) {
    return false;
  }

  byte raw = Wire.read();
  degrees = (int)(raw * 360.0 / 256.0);

  return true;
}

int angleDiff(int target, int current) {
  int diff = (target - current) % 360;

  if (diff > 180) {
    diff -= 360;
  }

  if (diff < -180) {
    diff += 360;
  }

  return diff;
}

void find_heading(int given_degree) {
  static unsigned long lastDisplay = 0;

  int current_heading;

  if (!readCompass(current_heading)) {
    stopMotors();
    lcd.setCursor(0, 0);
    lcd.print("Compass Error!      ");
    return;
  }

  int diff = angleDiff(given_degree, current_heading);
  int pwm = (abs(diff) < SLOW_ZONE) ? TURN_PWM_SLOW : TURN_PWM;

  const char* status;

  if (abs(diff) <= OFFSET) {
    stopMotors();
    status = "Status: Target OK   ";
  }
  else if (diff > 0) {
    turnRight(pwm);
    status = "Status: Turning R   ";
  }
  else {
    turnLeft(pwm);
    status = "Status: Turning L   ";
  }

  if (millis() - lastDisplay >= DISPLAY_MS) {
    lastDisplay = millis();

    lcd.setCursor(0, 0);
    lcd.print("Target: ");
    lcd.print(given_degree);
    lcd.print("     ");

    lcd.setCursor(0, 1);
    lcd.print("Current: ");
    lcd.print(current_heading);
    lcd.print("     ");

    lcd.setCursor(0, 2);
    lcd.print(status);

    Serial.print("Target: ");
    Serial.print(given_degree);
    Serial.print(" | Current: ");
    Serial.print(current_heading);
    Serial.print(" | Diff: ");
    Serial.println(diff);
  }
}

void turnRight(int pwm) {
  digitalWrite(Motor_L_dir_pin, Motor_forward);
  digitalWrite(Motor_R_dir_pin, Motor_return);

  analogWrite(Motor_L_pwm_pin, pwm);
  analogWrite(Motor_R_pwm_pin, pwm);
}

void turnLeft(int pwm) {
  digitalWrite(Motor_L_dir_pin, Motor_return);
  digitalWrite(Motor_R_dir_pin, Motor_forward);

  analogWrite(Motor_L_pwm_pin, pwm);
  analogWrite(Motor_R_pwm_pin, pwm);
}

void stopMotors() {
  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);
}