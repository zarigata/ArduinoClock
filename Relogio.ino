#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// Define the RX and TX pins for the DFPlayer Mini
SoftwareSerial mySoftwareSerial(4, 5); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000); // Set the offset to -3 hours for Brasília Standard Time

int previousHour = -1;
int previousMinute = -1;

void setup() {
  Serial.begin(115200);
  mySoftwareSerial.begin(9600);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize DFPlayer Mini
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1. Please recheck the connection!"));
    Serial.println(F("2. Please insert the SD card!"));
    while (true);
  }

  // Set the volume (0 to 30)
  myDFPlayer.volume(20);

  // Start NTP time synchronization
  timeClient.begin();
}

void loop() {
  timeClient.update();

  int currentHour24 = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  
  // Convert to 12-hour format
  int currentHour12 = (currentHour24 % 12 == 0) ? 12 : currentHour24 % 12;
  
  // Check if the hour has changed
  if (currentHour24 != previousHour) {
    playTrack(currentHour12); // Play track 1 every hour
    delay(60000);
    // Play track 2 as many times as the current hour
    for (int i = 0; i < currentHour24; i++) {
      playTrack(2);
      delay(3000);
    }

    previousHour = currentHour24;
  }

  // Check if the minute has changed
  if (currentMinute != previousMinute) {
    // Play track 3 every half-hour
    if (currentMinute % 30 == 0) {
      playTrack(3);
      delay(5000);
    }

    previousMinute = currentMinute;
  }

  delay(1000);
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
