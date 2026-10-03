#include <LiquidCrystal.h>

// 1. LCD Configuration (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

// 2. Motor Driver Pin Definitions
#define Motor_forward    0
#define Motor_return     1
#define Motor_L_dir_pin  8
#define Motor_R_dir_pin  7
#define Motor_L_pwm_pin  10
#define Motor_R_pwm_pin  9

// 3. Encoder Pin Definitions
const int encoderPinL = 2; // Left Encoder (Interrupt 0)
const int encoderPinR = 3; // Right Encoder (Interrupt 1)

// Interrupt Variables
volatile unsigned long pulseCountL = 0;
volatile unsigned long pulseCountR = 0;

// 4. CALIBRATION & COMPETITION SETTINGS
const float PULSES_PER_CM = 13.97; // Actual pulses per cm for your robot

const int baseSpeed = 150; 
float targetDistanceCm = 300.0; // 3 meters = 300 cm

// Interrupt Service Routines (ISRs)
void ISR_countPulseL() {
  pulseCountL++;
}

void ISR_countPulseR() {
  pulseCountR++;
}

void stopMotors() {
  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);
  digitalWrite(Motor_L_dir_pin, LOW);
  digitalWrite(Motor_R_dir_pin, LOW);
}

void setup() {
  lcd.begin(20, 4);

  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);

  pinMode(encoderPinL, INPUT_PULLUP);
  pinMode(encoderPinR, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(encoderPinL), ISR_countPulseL, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderPinR), ISR_countPulseR, RISING);

  lcd.setCursor(0, 0);
  lcd.print("Target: ");
  lcd.print(targetDistanceCm);
  lcd.print(" cm");
  delay(1500);

  driveTargetDistance(targetDistanceCm);
}

void loop() {
  // Keep the results on the LCD after stopping
}

void driveTargetDistance(float distanceCm) {
  unsigned long targetPulses = distanceCm * PULSES_PER_CM;

  pulseCountL = 0;
  pulseCountR = 0;

  digitalWrite(Motor_L_dir_pin, Motor_forward);
  digitalWrite(Motor_R_dir_pin, Motor_forward);

  // Start both motors running at base speed
  int currentLeftPwm = baseSpeed;
  int currentRightPwm = baseSpeed;
  
  analogWrite(Motor_L_pwm_pin, currentLeftPwm);
  analogWrite(Motor_R_pwm_pin, currentRightPwm);

  // Continuous monitoring loop with active encoder feedback correction
  while (true) {
    unsigned long currentAvgPulses = (pulseCountL + pulseCountR) / 2;

    // CHECK STOP CONDITION IMMEDIATELY
    if (currentAvgPulses >= targetPulses) {
      stopMotors(); // Cut power immediately
      break;
    }

    // Active drift correction based on encoder counts
    if (pulseCountR > pulseCountL) {
      // Right side is spinning faster (causing left turn) -> slow down right, speed up left
      currentRightPwm = max(100, baseSpeed - 20);
      currentLeftPwm  = min(255, baseSpeed + 10);
    } else if (pulseCountL > pulseCountR) {
      // Left side is spinning faster (causing right turn) -> slow down left, speed up right
      currentLeftPwm  = max(100, baseSpeed - 20);
      currentRightPwm = min(255, baseSpeed + 10);
    } else {
      // Both equal -> run at base speed
      currentLeftPwm  = baseSpeed;
      currentRightPwm = baseSpeed;
    }

    // Apply the updated PWM values to the motors
    analogWrite(Motor_L_pwm_pin, currentLeftPwm);
    analogWrite(Motor_R_pwm_pin, currentRightPwm);
  }

  // Print parameters to the LCD only after the robot has fully stopped to avoid slowing down interrupts
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("=== STOPPED ===");
  lcd.setCursor(0, 1);
  lcd.print("Target: "); lcd.print(distanceCm); lcd.print("cm");
  lcd.setCursor(0, 2);
  lcd.print("L: "); lcd.print(pulseCountL); 
  lcd.print(" | R: "); lcd.print(pulseCountR);
  lcd.setCursor(0, 3);
  lcd.print("Target P: "); lcd.print(targetPulses);
}