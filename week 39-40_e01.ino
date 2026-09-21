#include <LiquidCrystal.h>

#define Motor_forward    0
#define Motor_return     1
#define Motor_L_dir_pin  8 //we swapped the pins because the turning was inversed
#define Motor_R_dir_pin  7
#define Motor_L_pwm_pin  10
#define Motor_R_pwm_pin  9

LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

// PWM Speed Constants (8-bit: 0 - 255)
const int SPEED_30_PERCENT = 76;   // 255 * 0.30
const int SPEED_75_PERCENT = 191;  // 255 * 0.75

void stopMotors() {
  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);
  digitalWrite(Motor_L_dir_pin, LOW);
  digitalWrite(Motor_R_dir_pin, LOW);
}


void setup() {
  // Force pins LOW before anything else
  digitalWrite(Motor_L_pwm_pin, LOW);
  digitalWrite(Motor_R_pwm_pin, LOW);
  digitalWrite(Motor_L_dir_pin, LOW);
  digitalWrite(Motor_R_dir_pin, LOW);

  // Set pin modes
  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);

  // 3. Ensure outputs are zeroed
  stopMotors();

  //Step 1: left wheel forward
  digitalWrite(Motor_L_dir_pin, Motor_forward);
  analogWrite(Motor_L_pwm_pin, SPEED_30_PERCENT);
  analogWrite(Motor_R_pwm_pin, 0);
  delay(2000);

  // Stop before next step
  stopMotors();
  delay(150);

  //Step 2: right wheel forward
  
  digitalWrite(Motor_R_dir_pin, Motor_forward);
  analogWrite(Motor_R_pwm_pin, SPEED_30_PERCENT);
  analogWrite(Motor_L_pwm_pin, 0);
  delay(1000);

  // Stop & clear motor coils before reversing
  stopMotors();
  delay(200);

  //Step 3: both wheels backwards
  digitalWrite(Motor_L_dir_pin, Motor_return);
  digitalWrite(Motor_R_dir_pin, Motor_return);
  analogWrite(Motor_L_pwm_pin, SPEED_75_PERCENT);
  analogWrite(Motor_R_pwm_pin, SPEED_75_PERCENT);
  delay(4000);

  //Step 4: stop motors
  
  stopMotors();
}

void loop() {
  // Sequence runs once in setup()
}