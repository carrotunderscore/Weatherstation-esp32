#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

const char* ssid = "RobertGalaxyS21";      
const char* password = "Knasboll95"; 

const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883; 

WiFiClient espClient;
PubSubClient client(espClient);

int lightSensorPin = 25;
int humidityPin = 26;
int waterSensorPin = 33;

unsigned long startMillis;

void setup() {
  delay(1000);  
  Serial.begin(115200);
  pinMode(lightSensorPin, INPUT);
  pinMode(humidityPin, INPUT);
  pinMode(waterSensorPin, INPUT);

  connectToWiFi();
  client.setServer(mqttServer, mqttPort);  
  client.setCallback(callback);
  connectToMQTT();

  startMillis = millis(); 
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }  
  client.loop();
  
  int lightValue = analogRead(lightSensorPin);  
  int moistureValue = analogRead(humidityPin);  
  int waterLevel = analogRead(waterSensorPin);

  StaticJsonDocument<200> jsonDoc;
  jsonDoc["light"] = lightValue;  
  jsonDoc["moisture"] = moistureValue;
  jsonDoc["water"] = waterLevel;

  char jsonBuffer[512];
  serializeJson(jsonDoc, jsonBuffer);  

  if (client.connected()) {
    client.publish("esp32/sensors", jsonBuffer);
  } 
  Serial.print("Light intensity: ");
  Serial.println(lightValue);
  Serial.print("Soil Moisture Level: ");
  Serial.println(moistureValue);
  Serial.print("Water Sensor Reading: ");
  Serial.println(waterLevel);

  sendStatus();

  delay(10000); 
}

void sendStatus() {
  unsigned long currentMillis = millis();
  unsigned long uptimeSeconds = currentMillis / 1000;
  int hours = uptimeSeconds / 3600;
  int minutes = (uptimeSeconds % 3600) / 60;

  float humidity = analogRead(humidityPin); 

  if (isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  StaticJsonDocument<256> jsonDoc;
  jsonDoc["uptime"] = String(hours) + "h " + String(minutes) + "m";
  jsonDoc["humidity"] = humidity;
  jsonDoc["network"] = (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected";

  char jsonBuffer[512];
  serializeJson(jsonDoc, jsonBuffer);

  client.publish("weatherstation/status", jsonBuffer);

  Serial.println(jsonBuffer);
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);
  
  String receivedMessage;
  for (int i = 0; i < length; i++) {
    receivedMessage += (char)message[i];
  }
  Serial.println("Message: " + receivedMessage);
}

void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi");
}

void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    
    if (client.connect("ESP32Client_esp32")) {
      Serial.println("Connected to MQTT");
      client.subscribe("esp32/sensors");
    } else {
      Serial.print("Failed with state ");
      Serial.println(client.state());
      delay(2000);
    }

    if (client.connect("WeatherStationClient")) {
      Serial.println("Connected to MQTT");
      client.subscribe("weatherstation/commands");
    } else {
      Serial.print("Failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}
