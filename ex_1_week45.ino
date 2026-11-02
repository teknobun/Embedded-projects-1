#include <LiquidCrystal.h>

// Initialized with your specific pins: (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(37, 36, 35, 34, 33, 32);

void setup(){
  Serial.begin(9600);
  delay(1000);          // Allow serial communication to stabilize at boot
  lcd.begin(20, 4);     // Set for 20 columns and 4 rows
  
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
      else {
        Serial.println("No recognized command found, try typing Print:Hi or LCD:Hello\n");
      }
    }
  }
}