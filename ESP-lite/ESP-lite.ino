#include <ESP8266WiFi.h>        // Library for Wi-Fi functionality
#include <ESP8266WebServer.h>   // Library to create and manage a web server
#include <LittleFS.h>                 // Library for working with file systems (LittleFS)

const char* ssid = "Titenet-IoT";         // Wi-Fi network name (SSID)
const char* password = "7kDtaphg";  // Wi-Fi network password


ESP8266WebServer server(80);    // Create an instance of the WebServer on port 80 (default HTTP port)


void setup() {
  Serial.begin(9600); 

  
  // Initialize the file system (LittleFS) on the ESP8266
  if (!LittleFS.begin()) {        // Initialize and check success on LittleFS
    Serial.println("Error while mounting LittleFS"); 
    return; 
  }

  // Connect to the Wi-Fi network using the provided SSID and password
  Serial.print("\nConnecting to " + (String)ssid);
  WiFi.begin(ssid, password);               // Begin connecting to Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {   // Wait in a loop until the connection is successful
    delay(500);
    Serial.print(".");  
  } Serial.println("\nIP address: " + WiFi.localIP().toString()); // Print the IP address of the ESP8266 when connected


  // Set up the web pages (URLS) and files that the server will show/use when someone visits the site
  server.serveStatic("/", LittleFS, "/index.html");             // Serves and shows the main webpage (HTML file) when someone visits the home page
  server.serveStatic("/style.css", LittleFS, "/style.css");     // Serve the CSS file for styling
  server.serveStatic("/script.js", LittleFS, "/script.js");     // Serve the JavaScript file
  server.serveStatic("/favicon.ico", LittleFS, "/favicon.png"); // Serve a favicon (small icon) for the website


  // Define custom actions/function calls for specific URLs (ie when a button press from webpage requests a particular URL)
  server.on("/forwards5", [](){ handleMove(5); });      // When requesting URL "/forwards5", call the handleMove function with parameter 5
  server.on("/forwards20", [](){ handleMove(20); });    // [](){ handleMove(x); } is a lambda function that contains the code  
  server.on("/backwards5", [](){ handleMove(-5); });    // to be executed when the route is visited. These lambda functions call
  server.on("/backwards20", [](){ handleMove(-20); });  // handleMove(x) as soon as it's triggered.
  server.on("/compass", handleCompass);                 // When requesting URL "/compass", call the handleCompass function
  server.on("/findnorth", handleCompass2);              // Handling the find north button press


  //  If someone tries to access a URL that does not exist (e.g. due to a typo), call the handleNotFound function
  server.onNotFound(handleNotFound);


  // Start the web server
  server.begin();
}


void loop() {
  
  if (Serial.read() == 'r') {
  ESP.restart();
}
  server.handleClient();  // Listen for incoming HTTP requests and respond to them
}


// This function is called when a non-existing URL is accessed (404 error)
void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found"); // Send a 404 response with a plain text message
}


// This function handles movement commands like "/forwards5" or "/backwards20"
void handleMove(int distance) {
  Serial.println("Move:" + String(distance));       // Print the movement distance to the serial monitor for Arduino Mega
  server.send(200);                                 // Send a 200 OK response to the client (browser)
}


// This function handles the compass command (like "/compass?value=30")
void handleCompass() {
  if (server.hasArg("value")) {                     // Check if there is a "value" argument in the request URL
    String valueString = server.arg("value");       // Get the "value" argument from the URL
    Serial.println("Turn:" + valueString);          // Print the compass value to the serial monitor
  }
  server.send(200);
}
  
void handleCompass2() {
  if (server.hasArg("value")) {                     // Check if there is a "value" argument in the request URL
    String valueString = server.arg("value");       // Get the "value" argument from the URL
    Serial.println("TurnTo:" + valueString);          // Print the compass value to the serial monitor
  }
  server.send(200);
}
