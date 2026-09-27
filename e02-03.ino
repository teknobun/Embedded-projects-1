#include <LiquidCrystal.h>

#define Motor_forward    0
#define Motor_return     1

// Pin definitions matching Pololu Shield layout
#define Motor_L_dir_pin  8
#define Motor_R_dir_pin  7
#define Motor_L_pwm_pin  10
#define Motor_R_pwm_pin  9

LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

const int xPin = A8;
const int yPin = A9;

// Track current PWM
float currentLeftPWM = 0;
float currentRightPWM = 0;

// RACE TUNING PARAMETERS
const float MAX_ACCEL = 30.0;          // Fast acceleration
const float STEERING_SENSITIVITY = 1.2; // Sharper turns

// Independent Non-Blocking Timers
unsigned long lastMotorUpdate = 0;
const unsigned long motorInterval = 10; // 100Hz motor loop

unsigned long lastLcdUpdate = 0;
const unsigned long lcdInterval = 500;  // Low frequency LCD update

void stopMotors() {
  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);
  digitalWrite(Motor_L_dir_pin, LOW);
  digitalWrite(Motor_R_dir_pin, LOW);
  currentLeftPWM = 0;
  currentRightPWM = 0;
}

void setup() {
  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);

  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);

  stopMotors();

  lcd.begin(20, 4);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("== MAX SPEED MODE ==");
  lcd.setCursor(0, 2);
  lcd.print("Ready to Race!");
}

void loop() {
  unsigned long currentMillis = millis();

  // --- 1. ULTRA-FAST MOTOR LOOP (100Hz / 10ms) ---
  if (currentMillis - lastMotorUpdate >= motorInterval) {
    lastMotorUpdate = currentMillis;

    // Read analog joystick
    int rawX = analogRead(xPin);
    int rawY = 1023 - analogRead(yPin);

    // Deadzone filter around center
    int deadzone = 35;
    if (abs(rawX - 512) < deadzone) rawX = 512;
    if (abs(rawY - 512) < deadzone) rawY = 512;

    // Convert raw readings to signed motor drive ranges (-255 to 255)
    int throttle = map(rawY, 0, 1023, -255, 255);
    int steering = map(rawX, 0, 1023, -255, 255) * STEERING_SENSITIVITY;

    // Calculate differential steering speeds
    int targetLeft  = throttle + steering;
    int targetRight = throttle - steering;

    targetLeft  = constrain(targetLeft, -255, 255);
    targetRight = constrain(targetRight, -255, 255);

    // Fast Ramping
    if (targetLeft > currentLeftPWM)  currentLeftPWM = min((float)targetLeft, currentLeftPWM + MAX_ACCEL);
    else if (targetLeft < currentLeftPWM) currentLeftPWM = max((float)targetLeft, currentLeftPWM - MAX_ACCEL);

    if (targetRight > currentRightPWM) currentRightPWM = min((float)targetRight, currentRightPWM + MAX_ACCEL);
    else if (targetRight < currentRightPWM) currentRightPWM = max((float)targetRight, currentRightPWM - MAX_ACCEL);

    // Output Left Motor
    if (currentLeftPWM > 0) {
      digitalWrite(Motor_L_dir_pin, Motor_forward);
      analogWrite(Motor_L_pwm_pin, (int)currentLeftPWM);
    } else if (currentLeftPWM < 0) {
      digitalWrite(Motor_L_dir_pin, Motor_return);
      analogWrite(Motor_L_pwm_pin, (int)(-currentLeftPWM));
    } else {
      analogWrite(Motor_L_pwm_pin, 0);
    }

    // Output Right Motor
    if (currentRightPWM > 0) {
      digitalWrite(Motor_R_dir_pin, Motor_forward);
      analogWrite(Motor_R_pwm_pin, (int)currentRightPWM);
    } else if (currentRightPWM < 0) {
      digitalWrite(Motor_R_dir_pin, Motor_return);
      analogWrite(Motor_R_pwm_pin, (int)(-currentRightPWM));
    } else {
      analogWrite(Motor_R_pwm_pin, 0);
    }
  }

  // --- 2. LCD UPDATE LOOP (500ms) ---
  if (currentMillis - lastLcdUpdate >= lcdInterval) {
    lastLcdUpdate = currentMillis;

    int leftPct  = map(abs(currentLeftPWM), 0, 255, 0, 100);
    int rightPct = map(abs(currentRightPWM), 0, 255, 0, 100);

    lcd.setCursor(0, 3);
    lcd.print("L:"); lcd.print(leftPct); lcd.print("%  R:"); lcd.print(rightPct); lcd.print("%   ");
  }
}