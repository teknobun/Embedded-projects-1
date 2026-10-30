// ============================================================
//  Exercise 5: drive a triangle using encoder pulses + compass
//
//  Route (H0 = the direction the car is facing at the start,
//  it can be anything):
//    Leg 1: drive 20 cm at 75% speed
//    Turn : left 110 degrees   (heading H0 - 110)
//    Leg 2: drive 13 cm at 25% speed
//    Turn : left 110 degrees   (heading H0 - 220)
//    Leg 3: drive 20 cm at 50% speed  (back at the start spot)
//
//  - The ENCODERS measure how far the car has driven (pulse counting).
//  - The COMPASS tells when a turn is finished (same method as exercise 4).
//  - The route runs once at power-up, and once more every time the
//    joystick push button is pressed.
// ============================================================

#include <Wire.h>             // I2C communication (used by the compass)
#include <LiquidCrystal.h>    // LCD display


// ------------------------------------------------------------
//  LCD configuration (RS, E, D4, D5, D6, D7)
// ------------------------------------------------------------
LiquidCrystal lcd(37, 36, 35, 34, 33, 32);


// ------------------------------------------------------------
//  Compass configuration
// ------------------------------------------------------------
const byte CMPS14_ADDRESS = 0x60;   // I2C address of the CMPS14 compass
const byte REG_HEADING_8  = 0x01;   // register holding the 8-bit heading (0-255)


// ------------------------------------------------------------
//  Motor pins and directions
// ------------------------------------------------------------
#define Motor_forward    0    // level on the direction pin = forward
#define Motor_return     1    // level on the direction pin = backward
#define Motor_L_dir_pin  8    // left motor direction pin
#define Motor_R_dir_pin  7    // right motor direction pin
#define Motor_L_pwm_pin  10   // left motor speed pin (PWM)
#define Motor_R_pwm_pin  9    // right motor speed pin (PWM)


// ------------------------------------------------------------
//  Joystick push button
//  CHECK YOUR WIRING: pin 19 and "pressed = LOW" were taken from the
//  older exercise 5 sketch. Change JOY_SW if your button is elsewhere.
// ------------------------------------------------------------
const byte JOY_SW = 19;
const bool RUN_ON_STARTUP = true;   // true = drive once at power-up, then on each button press


// ------------------------------------------------------------
//  Encoder settings
//  CHECK YOUR WIRING: the pins must be interrupt pins on the Mega
//  (2, 3, 18, 19). Pins 20 and 21 are used by the compass (I2C),
//  and 19 is used by the joystick button above.
// ------------------------------------------------------------
const byte ENC_L_PIN = 2;              // left wheel encoder
const byte ENC_R_PIN = 3;              // right wheel encoder
const float PULSES_PER_CM = 13.5;      // CALIBRATE: drive 100 cm, divide the pulses by 100


// ------------------------------------------------------------
//  Route (from the assignment)
// ------------------------------------------------------------
const float LEG1_CM = 25.0;   const int LEG1_SPEED = 75;   // speed in percent of max
const float LEG2_CM = 17.0;   const int LEG2_SPEED = 25;
const float LEG3_CM = 15.0;   const int LEG3_SPEED = 50;
const int   TURN1_DEG = 95;                                // each left turn
const int   TURN2_DEG = 98;


// ------------------------------------------------------------
//  Turning settings (same idea as exercise 4)
// ------------------------------------------------------------
const int offset = 4;                       // allowed heading error: +/- 4 degrees
const int TURN_PWM = 255;                   // constant spin speed while turning
const int LOOP_DELAY_MS = 30;               // pause per compass check while turning
const unsigned long TURN_TIMEOUT_MS = 8000; // give up a turn after 8 s (safety)
// ------------------------------------------------------------
//  Driving settings
// ------------------------------------------------------------
const int KICK_BELOW_PWM = 100;             // below this PWM the car may not start moving...
const int KICK_PWM = 180;                   // ...so it first gets a short burst of this PWM
const int KICK_MS = 60;                     // length of that burst
const unsigned long LEG_TIMEOUT_MS = 10000; // give up a leg after 10 s (safety, e.g. encoder not counting)


// ------------------------------------------------------------
//  Encoder counters
//  volatile = changed inside an interrupt, so the compiler must not cache it
// ------------------------------------------------------------
volatile unsigned long encLeftCount  = 0;
volatile unsigned long encRightCount = 0;

// Interrupt routines: called automatically on every encoder pulse
void encLeftISR()  { encLeftCount++; }
void encRightISR() { encRightCount++; }


// ============================================================
//  setup(): runs once at power-up
// ============================================================
void setup() {
  lcd.begin(20, 4);        // start the LCD (20 columns, 4 rows)
  Wire.begin();            // start the Arduino as I2C controller (compass)
  Serial.begin(9600);      // Serial monitor, used to read the pulse counts

  // Motor pins are outputs
  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);
  stopMotors();

  // Joystick button: INPUT_PULLUP makes the pin read LOW when pressed
  pinMode(JOY_SW, INPUT_PULLUP);

  // Encoder pins are inputs, every rising edge calls the interrupt routine
  pinMode(ENC_L_PIN, INPUT_PULLUP);
  pinMode(ENC_R_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_L_PIN), encLeftISR, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_R_PIN), encRightISR, RISING);

  if (RUN_ON_STARTUP) {
    showStatus("Starting in 2 s...");
    delay(2000);           // time to put the car down on the floor
    runTriangle();         // drive the whole route once
  } else {
    showStatus("Press button");
  }
}


// ============================================================
//  loop(): waits for the joystick button, one run per press
// ============================================================
void loop() {
  if (digitalRead(JOY_SW) == LOW) {          // button pressed
    delay(30);                               // debounce
    if (digitalRead(JOY_SW) == LOW) {        // still pressed = real press
      runTriangle();                         // drive the route once
      while (digitalRead(JOY_SW) == LOW) { } // wait until the button is released
      delay(30);
    }
  }
}


// ============================================================
//  runTriangle(): the whole route, step by step
// ============================================================
void runTriangle() {

  // ----- Remember the start heading H0 (try 3 times in case a read fails) -----
  int h0 = -1;
  for (int i = 0; i < 3 && h0 == -1; i++) {
    h0 = readCompass();
  }
  if (h0 == -1) {
    showStatus("Compass Error!");
    return;                                  // no compass = no route
  }

  // All target headings are calculated from H0, so errors don't add up
  int heading2 = wrap360(h0 - TURN1_DEG);       // after turn 1
  int heading3 = wrap360(h0 - 2 * TURN2_DEG);   // after turn 2

  // ----- Leg 1 -----
  showStatus("Leg 1: 20cm 75%");
  driveForward(LEG1_CM, LEG1_SPEED);

  // ----- Turn 1 -----
  showStatus("Turn 1: left 110");
  turnToHeading(heading2);

  // ----- Leg 2 -----
  showStatus("Leg 2: 13cm 25%");
  driveForward(LEG2_CM, LEG2_SPEED);

  // ----- Turn 2 -----
  showStatus("Turn 2: left 110");
  turnToHeading(heading3);

  // ----- Leg 3 -----
  showStatus("Leg 3: 20cm 50%");
  driveForward(LEG3_CM, LEG3_SPEED);

  // ----- Done: the car is back at the start spot -----
  showStatus("Done. Press button");
}
// ============================================================
//  driveForward(): drive straight for distanceCm at speedPercent
//  The distance is measured by counting encoder pulses.
// ============================================================
void driveForward(float distanceCm, int speedPercent) {

  // How many pulses correspond to the wanted distance
  unsigned long targetPulses = (unsigned long)(distanceCm * PULSES_PER_CM);

  // Convert percent to a PWM value (75% -> 191, 50% -> 127, 25% -> 63)
  int pwm = speedPercent * 255 / 100;

  resetPulses();                           // start counting from 0
  unsigned long startTime = millis();      // for the safety timeout

  // Slow speeds are too weak to get the car moving from standstill,
  // so give the motors a short burst first
  if (pwm < KICK_BELOW_PWM) {
    forward(KICK_PWM);
    delay(KICK_MS);
  }

  forward(pwm);                            // drive at the wanted speed

  // Keep driving until the wanted number of pulses is reached
  while (getPulses() < targetPulses) {
    if (millis() - startTime > LEG_TIMEOUT_MS) {
      break;                               // safety: stop if the pulses never arrive
    }
  }

  stopMotors();

  // Print the result, useful for calibrating PULSES_PER_CM
  Serial.print("Leg done, pulses: ");
  Serial.println(getPulses());

  delay(300);                              // let the car settle before the next step
}


// ============================================================
//  turnToHeading(): spin on the spot until the compass shows target
//  Same method as exercise 4: shortest way, stops inside +/- offset.
// ============================================================
void turnToHeading(int target) {

  unsigned long startTime = millis();      // for the safety timeout

  while (true) {

    int current = readCompass();
    if (current == -1) {                   // compass error: stop for safety
      break;
    }

    int diff = angleDiff(target, current); // positive = target is to the right

    if (abs(diff) <= offset) {
      break;                               // close enough: turn finished
    }
    if (millis() - startTime > TURN_TIMEOUT_MS) {
      break;                               // safety: give up after too long
    }

    if (diff > 0) {
      turnRight();
    } else {
      turnLeft();
    }

    delay(LOOP_DELAY_MS);
  }

  stopMotors();
  delay(300);                              // let the car settle before the next step
}


// ============================================================
//  Encoder helper functions
// ============================================================

// Average pulse count of both wheels.
// Interrupts are paused while copying because the 4-byte counters are
// read in several steps on this 8-bit chip.
unsigned long getPulses() {
  noInterrupts();
  unsigned long l = encLeftCount;
  unsigned long r = encRightCount;
  interrupts();
  return (l + r) / 2;
}

// Set both counters back to 0
void resetPulses() {
  noInterrupts();
  encLeftCount  = 0;
  encRightCount = 0;
  interrupts();
}


// ============================================================
//  Compass helper functions (same as exercise 4)
// ============================================================

// Reads the compass and returns the heading in degrees (0-359), or -1 on error
int readCompass() {
  Wire.beginTransmission((int)CMPS14_ADDRESS);
  Wire.write(REG_HEADING_8);
  if (Wire.endTransmission(false) != 0) {
    return -1;             // I2C communication error
  }

  Wire.requestFrom((int)CMPS14_ADDRESS, 1);
  if (Wire.available() >= 1) {
    byte heading8 = Wire.read();                 // 8-bit value, 0-255
    return (int)(heading8 * 360.0 / 256.0);      // 256 steps = 360 degrees
  }
  return -1;               // read failure
}
// Shortest way from current to target: -180..+180
// positive = target is to the right, negative = target is to the left
int angleDiff(int target, int current) {
  int diff = (target - current) % 360;
  if (diff > 180)  diff -= 360;
  if (diff < -180) diff += 360;
  return diff;
}

// Wrap any angle into 0..359 (example: -110 becomes 250)
int wrap360(int angle) {
  return ((angle % 360) + 360) % 360;
}


// ============================================================
//  Motor helper functions
// ============================================================

// Both wheels forward at the given PWM
void forward(int pwm) {
  digitalWrite(Motor_L_dir_pin, Motor_forward);
  digitalWrite(Motor_R_dir_pin, Motor_forward);
  analogWrite(Motor_L_pwm_pin, pwm);
  analogWrite(Motor_R_pwm_pin, pwm);
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


// ============================================================
//  LCD helper: clears the screen and shows one line of text on row 0
// ============================================================
void showStatus(const char* text) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(text);
}