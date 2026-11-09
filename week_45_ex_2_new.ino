#include <LiquidCrystal.h>

// Initialized with specific pins: (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

// NEW: motor pins, needed to drive the car
#define Motor_forward    0
#define Motor_return     1
#define Motor_L_dir_pin  8
#define Motor_R_dir_pin  7
#define Motor_L_pwm_pin  10
#define Motor_R_pwm_pin  9

// NEW: encoder settings (use the pins and pulses/cm from your earlier driving exercise)
const byte ENC_L_PIN = 2;
const byte ENC_R_PIN = 3;
const float PULSES_PER_CM = 13.5;
const int DRIVE_PWM = 200;          // NEW: driving speed (0-255)

// NEW: pulse counters, changed inside interrupts so they must be volatile
volatile unsigned long encLeftCount  = 0;
volatile unsigned long encRightCount = 0;
void encLeftISR()  { encLeftCount++; }
void encRightISR() { encRightCount++; }

void setup(){
  Serial.begin(9600);
  delay(1000);          // Allow serial communication to stabilize at boot
  lcd.begin(20, 4);     // Set for 20 columns and 4 rows

  // NEW: motor pins are outputs, encoder pins trigger an interrupt on every pulse
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
}

void loop(){
  if (Serial.available() > 0){
    String message = Serial.readStringUntil('\n');
    Serial.print("Message received, content: ");
    Serial.println(message);

    int pos_s = message.indexOf("Print");
    if (pos_s > -1){
      Serial.println("Command = Print ");
      pos_s = message.indexOf(":");

      if (pos_s > -1){
        String stat = message.substring(pos_s + 1);
        if (stat == "Hi" || stat == "hi") {
          Serial.println("Hi!");
        }
        else if (stat == "Hello") {
          Serial.println("Hello there!");
        }
      }
    }
    else {
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
      // NEW: dist command, e.g. dist:10 drives forward, dist:-10 drives backward
      else if (message.indexOf("dist") > -1) {
        Serial.println("Command = dist");
        int pos_colon = message.indexOf(":");

        if (pos_colon > -1) {
          int value = message.substring(pos_colon + 1).toInt();  // NEW: number after the colon

          lcd.clear();                    // NEW: print the entire command on the LCD
          lcd.setCursor(0, 0);
          lcd.print(message);

          Serial.print("Driving cm: ");
          Serial.println(value);
          drive(value);                   // NEW: positive = forward, negative = backward
        }
      }
      else {
        // CHANGED: hint text also mentions the new command
        Serial.println("No recognized command found, try typing Print:Hi, LCD:Hello or dist:10\n");
      }
    }
  }
}

// NEW: drives the given number of cm using encoder pulses
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
  // (10 s timeout so the car stops even if an encoder is not counting)
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
