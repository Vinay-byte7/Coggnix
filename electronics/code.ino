#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <SoftwareSerial.h>

// --- Pin Assignments ---
#define DHTPIN 9           // Digital pin connected to DHT sensor
#define DHTTYPE DHT11      // DHT 11 model
const int mq135Pin = A0;   // Analog pin connected to MQ135 gas sensor
const int peltierPin = 11; // PWM pin connected to Peltier driver (3, 5, 6, 9, 10, or 11)
const int motorPin = 6;    // Digital pin for motor control
const int irPin = 12;      // Digital pin connected to IR sensor OUT

float sum = 0;
int n = 0;
float mean = 0;

// --- Display & Sensor Initializations ---
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(128, 64, &Wire, -1); // Screen size 128x64

// --- Control Variables ---
float targetTemp = 20.0; // Target temperature in °C

void setup() {
  Serial.begin(9600);

  // Initialize Sensors & Output Pins
  dht.begin();
  pinMode(peltierPin, OUTPUT);
  pinMode(motorPin, OUTPUT);
  pinMode(irPin, INPUT); // IR Sensor input pin setup

  // Initialize OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 OLED allocation failed"));
    for (;;); // Stop execution if display fails
  }

  // Display initial startup message
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("System Ready..."));
  display.display();
  delay(1000);
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int gasValue = analogRead(mq135Pin);
  
  // IR Sensor Read (Active LOW: LOW means obstacle detected)
  bool obstacleDetected = (digitalRead(irPin) == LOW); 

  // Handling sensor read failure
  if (isnan(temperature) || isnan(humidity)) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("DHT11 Error!");
    display.display();

    Serial.println("DHT11 Error! Failed to read sensor.");

    analogWrite(peltierPin, 0);
    delay(2000);
    return;
  }

  // -------- GAS ALERT & MEAN CALCULATION --------
  bool Fungus = false; 
  sum = sum + gasValue;
  n = n + 1;
  mean = sum / n;
  if (mean - 10 > gasValue || gasValue > mean + 10) {
    Fungus = true;
  }
  if (n > 30) {
    n = 0;
    sum = 0;
  }

  // -------- PELTIER CONTROL --------
  float error = temperature - targetTemp;
  int peltierPWM = 150;
  if (error > 2.0) {
    peltierPWM = constrain(error * 50, 0, 255);
  }
  analogWrite(peltierPin, peltierPWM);

  // -------- SERIAL MONITOR OUTPUT --------
  Serial.print("Temp: ");
  Serial.print(temperature, 1);
  Serial.print(" C | Hum: ");
  Serial.print(humidity, 1);
  Serial.print(" % | Gas: ");
  Serial.print(gasValue);
  Serial.print(Fungus ? " (HIGH!)" : "");
  Serial.print(" | Cool: ");
  Serial.print((peltierPWM * 100) / 255);
  Serial.print(" | IR : ");
  Serial.println(obstacleDetected ? "OBJECT DETECTED" : "Clear");

  // -------- OLED DISPLAY --------
  display.clearDisplay();
  display.setCursor(0, 0);

  display.print("Temp: ");
  display.print(temperature, 1);
  display.println(" C");

  display.print("Hum : ");
  display.print(humidity, 1);
  display.println(" %");

  display.print("Gas : ");
  display.print(gasValue);
  if (Fungus) display.print(" (HIGH)");
  display.println();

  display.print("Cool: ");
  display.print((peltierPWM * 100) / 255);
  display.println(" %");

  display.print("IR  : ");
  display.println(obstacleDetected ? "DETECTED" : "Clear");

  display.display();

  delay(1000);
}