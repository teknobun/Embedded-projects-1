#include <LiquidCrystal.h>

#define Motor_forward    0
#define Motor_return     1
#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10

LiquidCrystal lcd(37, 36, 35, 34, 33, 32);
int xPin = A8;
int yPin = A9;
int buttonPin = 19;
int xVal;
int yVal;
int xVal2;
int yVal2;
int perx;
int pery;

volatile int buttonCount = 0;
volatile unsigned long lastTime = 0;
int internalCount = -1;

void buttonPressed() {
  unsigned long interruptTime = millis();
  if (interruptTime - lastTime > 150) {
    buttonCount++;
    lastTime = interruptTime;
  }
}

void setup() {
  lcd.begin(20, 4);
  Serial.begin(9600);
  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP); 
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonPressed, FALLING);

  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);

  // Ensure motors start off
  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);
}

void runMotorSequenceOnce() {
  // 1. Move Return / Reverse Ramp Down
  digitalWrite(Motor_R_dir_pin, Motor_return);
  digitalWrite(Motor_L_dir_pin, Motor_return);
  for (int pwm = 150; pwm > 50; pwm--) {
    analogWrite(Motor_L_pwm_pin, pwm);
    analogWrite(Motor_R_pwm_pin, pwm);
    delay(50);
  }

  // 2. Move Forward Ramp Up
  digitalWrite(Motor_R_dir_pin, Motor_forward);
  digitalWrite(Motor_L_dir_pin, Motor_forward);
  for (int pwm = 50; pwm < 150; pwm++) {
    analogWrite(Motor_L_pwm_pin, pwm);
    analogWrite(Motor_R_pwm_pin, pwm);
    delay(50);
  }

  // 3. Stop motors after single run
  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);
}

void loop() {
  // Check if a new button press occurred
  if (internalCount != buttonCount) {
    lcd.clear(); 
    internalCount = buttonCount;

    if (buttonCount % 2 == 0) {
      // Normal display layout
      lcd.setCursor(0,0);  lcd.print("x: ");
      lcd.setCursor(0,2);  lcd.print("y: ");
      lcd.setCursor(9,0);  lcd.print("xper:");
      lcd.setCursor(9,2);  lcd.print("yper:");
      lcd.setCursor(17,0); lcd.print("%");
      lcd.setCursor(17,2); lcd.print("%");
    } else {
      // Push counter screen & trigger motor run ONCE
      lcd.setCursor(0, 0);
      lcd.print("Motor testing");
      
      // Run the motor pass once upon entering this state
      runMotorSequenceOnce();
    }
  }

  // Continuous screen updates based on current button count state
  if (buttonCount % 2 == 0) {
    xVal = analogRead(xPin);
    yVal = analogRead(yPin);
    yVal = 1023 - yVal;
    xVal2 = xVal - 511.5;
    yVal2 = yVal - 511.5;
    if (abs(xVal2) < 10) xVal2 = 0;
    if (abs(yVal2) < 10) yVal2 = 0;

    perx = (xVal / 1023.0) * 100;
    pery = (yVal / 1023.0) * 100;

    perx = constrain(perx, 0, 100);
    pery = constrain(pery, 0, 100);

    lcd.setCursor(3, 0);  lcd.print("     "); lcd.setCursor(3, 0);  lcd.print((int)xVal2);
    lcd.setCursor(3, 2);  lcd.print("     "); lcd.setCursor(3, 2);  lcd.print((int)yVal2);
    lcd.setCursor(14, 0); lcd.print("   ");   lcd.setCursor(14, 0); lcd.print(perx);
    lcd.setCursor(14, 2); lcd.print("   ");   lcd.setCursor(14, 2); lcd.print(pery);
  } else {
    lcd.setCursor(0, 1);  lcd.print("      ");
    lcd.setCursor(0, 1);  lcd.print(buttonCount);
  }

  delay(100);
}