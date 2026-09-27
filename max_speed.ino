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

// =========================================================
// HIGH-SPEED RACE TUNING PARAMETERS
// =========================================================
const float MAX_ACCEL = 255.0;          // Instant full throttle (no ramping delay)
const float STEERING_SENSITIVITY = 1.8; // 1.8x boost for aggressive, quick turns
const int DEADZONE = 25;               // Tightened deadzone for immediate response

// Timers
unsigned long lastMotorUpdate = 0;
const unsigned long motorInterval = 5;  // 200Hz hyper-responsive loop (5ms)

unsigned long lastLcdUpdate = 0;
const unsigned long lcdInterval = 1000; // 1Hz display refresh

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
  lcd.print("== HYPER RACE MODE ==");
  lcd.setCursor(0, 2);
  lcd.print("MAX TORQUE READY");
}

void loop() {
  unsigned long currentMillis = millis();

  // --- 1. HYPER-FAST MOTOR CONTROL LOOP (200Hz / 5ms) ---
  if (currentMillis - lastMotorUpdate >= motorInterval) {
    lastMotorUpdate = currentMillis;

    // Read analog joystick
    int rawX = analogRead(xPin);
    int rawY = 1023 - analogRead(yPin);

    // Tight deadzone check
    if (abs(rawX - 512) < DEADZONE) rawX = 512;
    if (abs(rawY - 512) < DEADZONE) rawY = 512;

    // Convert raw readings to full signed motor ranges (-255 to 255)
    int throttle = map(rawY, 0, 1023, -255, 255);
    int steering = map(rawX, 0, 1023, -255, 255) * STEERING_SENSITIVITY;

    // Calculate aggressive differential steering target
    int targetLeft  = throttle + steering;
    int targetRight = throttle - steering;

    // Constrain outputs to valid PWM bounds (-255 to 255)
    targetLeft  = constrain(targetLeft, -255, 255);
    targetRight = constrain(targetRight, -255, 255);

    // Instant Response Acceleration
    if (targetLeft > currentLeftPWM)  currentLeftPWM = min((float)targetLeft, currentLeftPWM + MAX_ACCEL);
    else if (targetLeft < currentLeftPWM) currentLeftPWM = max((float)targetLeft, currentLeftPWM - MAX_ACCEL);

    if (targetRight > currentRightPWM) currentRightPWM = min((float)targetRight, currentRightPWM + MAX_ACCEL);
    else if (targetRight < currentRightPWM) currentRightPWM = max((float)targetRight, currentRightPWM - MAX_ACCEL);

    // Output Left Motor (Direct PWM & Active Reversal for Tight Turns)
    if (currentLeftPWM > 0) {
      digitalWrite(Motor_L_dir_pin, Motor_forward);
      analogWrite(Motor_L_pwm_pin, (int)currentLeftPWM);
    } else if (currentLeftPWM < 0) {
      digitalWrite(Motor_L_dir_pin, Motor_return);
      analogWrite(Motor_L_pwm_pin, (int)(-currentLeftPWM));
    } else {
      analogWrite(Motor_L_pwm_pin, 0);
    }

    // Output Right Motor (Direct PWM & Active Reversal for Tight Turns)
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

  // --- 2. MINIMAL OVERHEAD LCD (1Hz) ---
  if (currentMillis - lastLcdUpdate >= lcdInterval) {
    lastLcdUpdate = currentMillis;

    int leftPct  = map(abs(currentLeftPWM), 0, 255, 0, 100);
    int rightPct = map(abs(currentRightPWM), 0, 255, 0, 100);

    lcd.setCursor(0, 3);
    lcd.print("L:"); lcd.print(leftPct); lcd.print("%  R:"); lcd.print(rightPct); lcd.print("%   ");
  }
}