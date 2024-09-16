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

void setup() {
  delay(1000);  
  Serial.begin(115200);

  connectToWiFi();
  client.setServer(mqttServer, mqttPort);  
  client.setCallback(callback);
  connectToMQTT();
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }  
  client.loop();

  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n'); 
    command.trim();  

    if (command == "status") {
      pullStatusFromBroker();  
    } else {
      Serial.println("Unknown command. Please type 'status' to pull data.");
    }
  }

  delay(1000);
}

void pullStatusFromBroker() {
  Serial.println("Requesting status from MQTT broker...");
  client.subscribe("weatherstation/status");  
}

void sendDebugCommand(String command) {
  StaticJsonDocument<128> jsonDoc;
  jsonDoc["command"] = command;

  char jsonBuffer[128];
  serializeJson(jsonDoc, jsonBuffer);

  client.publish("weatherstation/commands", jsonBuffer);
  Serial.println("Debug command sent: " + command);
}

void callback(char* topic, byte* message, unsigned int length) {  
  String receivedMessage;   
  for (int i = 0; i < length; i++) {
    receivedMessage += (char)message[i];
  }
  Serial.println("Message: " + receivedMessage);

  if (String(topic) == "status") {
    Serial.print("WEATHER STATION STATUS: ");
    Serial.println(receivedMessage);
  }
  client.unsubscribe("weatherstation/status");
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
    
    if (client.connect("ESP32Client_esp32_server")) {
      Serial.println("Connected to MQTT");
      
      if (client.subscribe("esp32/sensors")) {
        Serial.println("Subscribed to topic: esp32/sensors");
      } else {
        Serial.println("Failed to subscribe to topic: esp32/sensors");
      }
    } else {
      Serial.print("Failed with state ");
      Serial.println(client.state());
      delay(2000);
    }

    if (client.subscribe("weatherstation/commands")) {
        Serial.println("Subscribed to topic: weatherstation/commands");
      } else {
        Serial.println("Failed to subscribe to topic: weatherstation/commands");
      }
  }
}
