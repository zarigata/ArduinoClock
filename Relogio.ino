#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <Servo.h>

//====================================//
// Wi-Fi and Host Configuration
const char* ssid = "WIFI";
const char* password = "password";
const char* newHostname = "Relogio";

// SoftwareSerial for DFPlayer Mini
SoftwareSerial mySoftwareSerial(4, 5); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// Wi-Fi and NTP Configuration
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000);

// Servo Configuration
Servo myServo;
const int servoPin = 2; // Change this to the pin your servo is connected to

// Previous Hour and Minute
int previousHour = -1;
int previousMinute = -1;

// Volume Control Variables
const int nightVolume = 8; // Volume during night hours (10 PM to 8 AM)
const int dayVolume = 25;  // Volume during day hours

//====================================//

void setup() {
  Serial.begin(115200);
  mySoftwareSerial.begin(9600);
  
  // OTA and Wi-Fi Setup
  ArduinoOTA.setHostname(newHostname);
  ArduinoOTA.begin();
  WiFi.mode(WIFI_STA);
  WiFi.hostname(newHostname);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  Serial.println("Connected to WiFi");

  // DFPlayer Mini Initialization
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1. Please recheck the connection!"));
    Serial.println(F("2. Please insert the SD card!"));
    while (true);
  }

  myDFPlayer.volume(dayVolume); // Set initial volume

  // NTP Time Synchronization
  timeClient.begin();
  Serial.print("FILES IN SD CARD:");
  Serial.println(myDFPlayer.readFileCounts());

  // Servo Attachment
  myServo.attach(servoPin);
}

void loop() {
  // OTA and Time Update
  digitalWrite(LED_BUILTIN, HIGH);
  ArduinoOTA.handle();
  timeClient.update();

  int currentHour24 = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  int currentSecond = timeClient.getSeconds();

  int currentHour12 = (currentHour24 % 12 == 0) ? 12 : currentHour24 % 12;

  // Check if the hour has changed
  if (currentHour24 != previousHour) {
    playTrack(currentHour12);
    delay(85000);

    // Play track 2 as many times as the current hour
    for (int i = 0; i < currentHour12; i++) {
      myDFPlayer.play(13);
      delay(5000);
    }

    // Adjust volume based on time
    if (isNightTime(currentHour24)) {
      myDFPlayer.volume(nightVolume);
    } else {
      myDFPlayer.volume(dayVolume);
    }

    previousHour = currentHour24;
  }

  // Check if the minute has changed
  if (currentMinute != previousMinute) {
    // Play track 3 every half-hour
    if (currentMinute != 0 && currentMinute % 30 == 0) {
      myDFPlayer.play(14);
    }

    previousMinute = currentMinute;
  }

  // Map the current second (0-59) to the servo position (0-180)
  int servoPosition = map(currentMinute, 0, 59, 0, 180);

  // Set the servo to the calculated position
  myServo.write(servoPosition);

  delay(60000);
}

void playTrack(int trackNumber) {
  Serial.println("Playing track " + String(trackNumber));
  myDFPlayer.play(trackNumber);

  // Wait for the track to finish
  delay(1000);
  while (myDFPlayer.available()) {
    delay(1000); // Wait for a short time to avoid false positives
    if (!myDFPlayer.readState()) {
      // Track has finished playing
      break;
    }
  }
}

bool isNightTime(int hour) {
  // Check if the hour is between 10 PM and 8 AM
  return (hour >= 21 || hour < 8);
}
