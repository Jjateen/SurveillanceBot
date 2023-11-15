#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include "HX710B.h"
#include <Servo.h>  // Include the Servo library

// Replace with your network credentials
const char* ssid = "jjj";
const char* password = "12345678";

HX710B pressure_sensor;
long offset = 1855509;

// The TinyGPSPlus object
TinyGPSPlus gps;

// Define the analog pins for the sensors
const int mq2Pin = 5;  // Change this to the actual pin for the MQ-2 sensor

// Define the pins for the pressure sensor
const byte MPS_OUT_pin = 18;  // OUT data pin
const byte MPS_SCK_pin = 19;  // clock data pin

// Create instances of the WebServer and the HX711 pressure sensor
WebServer server(80);
Servo servoMotor;  // Create a Servo object to control the servo motor

float pressureValue = 0.0; // Declare pressureValue as a global variable
int servoAngle = 90;  // Initial servo angle

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);
  pinMode(mq2Pin,INPUT);
  pressure_sensor.begin(MPS_OUT_pin, MPS_SCK_pin, 128);
  pressure_sensor.set_offset(offset);
  servoMotor.attach(26);  // Attach the servo to pin 26

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(".");
  Serial.println("Connected to WiFi");
  Serial.print(WiFi.localIP());
  // Define server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/servo", HTTP_GET, handleServo);

  // Start the server
  server.begin();
}

void loop() {
  server.handleClient();

  // Read the analog value from the MQ-2 sensor
  int mq2Value = analogRead(mq2Pin);
  Serial.println("MQ-2 Gas Sensor Reading: " + String(mq2Value));

  // Read the pressure sensor
  // Set the scale
  pressureValue = pressure_sensor.is_ready() ? pressure_sensor.pascal() : 0;
  Serial.println("Pressure Sensor Reading: " + String(pressureValue, 2) + " kPa");

  while (Serial2.available() > 0) {
    if (gps.encode(Serial2.read())) {
      displayInfo();
    }
  }

  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS detected: check wiring."));
    while (true);
  }
  Serial.println(servoAngle);
}

void displayInfo() {
  Serial.print(F("Location: "));
  if (gps.location.isValid()) {
    Serial.print("Lat: ");
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print("Lng: ");
    Serial.println(gps.location.lng(), 6);
  } else {
    Serial.println(F("INVALID"));
  }
}

void handleRoot() {
  // Create a simple HTML page with an embedded map, sensor readings, and a servo motor control slider
  String html = "<html><head>";
  html += "<link rel='stylesheet' href='https://unpkg.com/leaflet@1.7.1/dist/leaflet.css' />";
  html += "<style>";
  html += "body { background-color: #242b3a; color: white; font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 0; }";
  html += ".info { display: inline-block; margin: 0 10px; }";
  html += "h1 { text-align: center; }";
  html += "#map-container { width: 600px; height: 600px; margin: 0 auto; border-radius: 20px; overflow: hidden; }";
  html += "#map { width: 100%; height: 100%; }";
  html += ".slider-container { margin-top: 20px; }"; // Add some margin for the slider
  html += "</style>";
  html += "</head><body>";
  html += "<h1>Surveillance Bot Data</h1>";
  html += "<div class='info'><p>Latitude: " + String(gps.location.lat(), 6) + "</p></div>";
  html += "<div class='info'><p>Longitude: " + String(gps.location.lng(), 6) + "</p></div>";
  html += "<div class='info'><p>MQ-2 Gas Sensor Reading: " + String(analogRead(mq2Pin)) + "</p></div>";
  html += "<div class='info'><p>Pressure Sensor Reading: " + String(pressureValue, 2) + " kPa</p></div>";
  html += "<div class='slider-container'>";
  html += "<p>Servo Motor Control:</p>";
  html += "<input type='range' id='servoSlider' min='0' max='180' step='1' value='" + String(servoAngle) + "' oninput='updateServo(this.value)' />";
  html += "<span id='sliderValue'>" + String(servoAngle) + "</span>";
  html += "</div>";
  html += "<div id='map-container'><div id='map'></div></div>";
  html += "<script src='https://unpkg.com/leaflet@1.7.1/dist/leaflet.js'></script>";
  html += "<script>";
  html += "var map = L.map('map').setView([" + String(gps.location.lat(), 6) + ", " + String(gps.location.lng(), 6) + "], 13);";
  html += "L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', { attribution: '© <a href=\"https://www.openstreetmap.org/copyright\">OpenStreetMap</a> contributors' }).addTo(map);";
  html += "L.marker([" + String(gps.location.lat(), 6) + ", " + String(gps.location.lng(), 6) + "]).addTo(map);";
  html += "function updateServo(sliderValue) { document.getElementById('sliderValue').innerText = sliderValue; var xhr = new XMLHttpRequest(); xhr.open('GET', '/servo?angle=' + sliderValue, true); xhr.send(); }";
  html += "</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleServo() {
  if (server.args() > 0) {
    String angle = server.arg("angle");
    int newAngle = angle.toInt();
    
    if (newAngle >= 0 && newAngle <= 180) {
      servoAngle = newAngle;
      servoMotor.write(servoAngle);
      server.send(200, "text/plain", "Servo angle set to " + angle);
    } else {
      server.send(400, "text/plain", "Invalid servo angle");
    }
  } else {
    server.send(400, "text/plain", "Missing servo angle parameter");
  }
}
