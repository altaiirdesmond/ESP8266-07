/*
  The code is designed for esp8266 to recieve serial data from uno

  circuit:
  ESP8266-01

  created:12/30/18

  @author: cdtekk
*/

#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <FirebaseArduino.h>
#include <ArduinoJson.h>

// Listen at port 80
ESP8266WebServer webserver(80);

// Firebase
FirebaseArduino firebase;
const char *FIREBASE_HOST = "projectespiot.firebaseio.com";
const char *FIREBASE_AUTH = "sqffOzK95EZd7d0jJFJTy65m0XqnwZqLEo8RAurB";

// WiFi credentials
const char *ssid = "Thesis";
const char *password = "aratan3525";

long t;
unsigned long lastDisplayUpdate = 0;
int fan1State = 0;
int fan2State = 0;
bool firstInit = true;

String content;
String fileContent;
String sensorContent;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(74880);

  // Connect to WiFi network
  WiFi.begin(ssid, password);
  WiFi.BSSID();
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }

  firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  fan1State = firebase.getInt(F("/sensor/Fan1"));
  fan2State = firebase.getInt(F("/sensor/Fan2"));

  webserver.on(F("/reading"), handleRootPath);
  webserver.on(F("/hwRestart"), handleRestartPath);
  webserver.begin();

  // Inform arduino of esp8266's IP
  Serial.print(F("IP: "));
  Serial.println(WiFi.localIP().toString());
}

void loop() {
  if (Serial.available()) {
    content = Serial.readString();
    //Serial.println(content);
    if (content.startsWith(F("reading"))) {
      sensorContent = content;

      // Clean
      sensorContent.replace(F("reading"), F(""));
      sensorContent.trim();

      // Cache json object from serial data
      DynamicJsonBuffer jsonBuffer;
      JsonObject &object = jsonBuffer.parseObject(sensorContent);

      // Then add new roots(fan1 and fan2)
      DynamicJsonBuffer newJsonBuffer;
      JsonObject &root = newJsonBuffer.createObject();

	  // fetching temp for setting up fan state
	  int temp = object["temperature"];

      root["temperature"] = temp;
      root["humidity"] = object["humidity"];
      root["time"] = object["time"];

	  if (temp <= 34) {
		  root["fan1"] = 0;
		  root["fan2"] = 0;
	  }
	  else if (temp > 34) {
		  root["fan1"] = 1;
		  root["fan2"] = 1;
	  }

      // Push to database
      firebase.push(F("/sensor/dht"), root);
      if (!firebase.success()) {
        Serial.println(firebase.error());
        return;
      }

      root.printTo(Serial);

      digitalWrite(LED_BUILTIN, HIGH);
      delay(50);
      digitalWrite(LED_BUILTIN, LOW);
      delay(50);
    }
  }

  unsigned long currentMillis = millis(); // Gets track of time
  if ((unsigned long)(currentMillis - lastDisplayUpdate) >= 1000) {
    if (firebase.getInt(F("/sensor/Fan1")) != fan1State ||
        firebase.getInt(F("/sensor/Fan2")) != fan2State || firstInit) {
      firstInit = false;
      fan1State = firebase.getInt(F("/sensor/Fan1"));
      fan2State = firebase.getInt(F("/sensor/Fan2"));

      String fanState = F("FanControl:");
      fanState += String(firebase.getInt(F("sensor/Fan1"))) + F(",") +
                  String(firebase.getInt(F("/sensor/Fan2")));

      Serial.println(fanState);
    }
  }

  webserver.handleClient();
}

// Where the sensor values will be periodically sent
void handleRootPath() {
  webserver.send(200, F("text/plain"), sensorContent);

  Serial.println(sensorContent);
}

void handleRestartPath() {
  webserver.send(200, F("text/plain"), F("ESP restart"));
  Serial.println(F("restart"));
  ESP.restart();
}
