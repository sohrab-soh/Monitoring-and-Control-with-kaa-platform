#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "DHT.h"
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

float hum;
float temp;

const char* ssid = "TP-LINK_6EE718";        // WiFi name
const char* password = "sohr_@_b";    // WiFi password
const char* mqtt_server = "mqtt.cloud.kaaiot.com";
const String TOKEN = "0y9f8QqvzJ";        // Endpoint token - you get (or specify) it during device provisioning
const String APP_VERSION = "c098esc8572kh3s4gqsg-v1";  // Application version - you specify it during device provisioning




const unsigned long fiveSeconds = 1 * 5 * 1000UL;
static unsigned long lastPublish = 0 - fiveSeconds;
const int LAMP = 12;
const int FAN = 13;
const int HEATER = 5;
const char* ON = "on";
const char* OFF = "off";

boolean hasRun = false;
boolean automate = false;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  pinMode(LAMP,OUTPUT);
  pinMode(FAN,OUTPUT);
  pinMode(HEATER,OUTPUT);

  Serial.begin(115200);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  dht.begin();   
}

void loop() {
  setup_wifi();
  if (!client.connected()) {
    reconnect();
  }
  if(hasRun==false){
      setDeafaultState();
      hasRun = true;
    }
  
  client.loop();
  unsigned long now = millis();
  if (now - lastPublish >= fiveSeconds) {
    lastPublish += fiveSeconds;
    DynamicJsonDocument telemetry(1023);
    telemetry.createNestedObject();

    hum = dht.readHumidity();
    temp= dht.readTemperature();
    
    if(automate==true){
       if(temp < 20.0){
        digitalWrite(HEATER,HIGH);
        telemetry[0]["Heater"] = 1;
       }
       if(temp > 25){
        digitalWrite(HEATER,LOW);
        telemetry[0]["Heater"] = 0;
       }
       if(temp > 32){
        digitalWrite(FAN,HIGH);
        telemetry[0]["Fan"] = 1;
       }
       if(temp < 30){
        digitalWrite(FAN,LOW);
        telemetry[0]["Fan"] = 0;
       }
    }
    telemetry[0]["temperature"] = temp; 
    telemetry[0]["humidity"] = hum;
    
    String topic = "kp1/" + APP_VERSION + "/dcx/" + TOKEN + "/json";
    client.publish(topic.c_str(), telemetry.as<String>().c_str());
    Serial.println("Published on topic: " + topic);
    Serial.println(hum);
    Serial.println(temp);
    
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("\nHandling command message on topic: %s\n", topic);
  String switchTopic = "kp1/" + APP_VERSION + "/cex/" + TOKEN + "/command/SWITCH/status";
  Serial.println("switchTopic is runing");
  DynamicJsonDocument doc(1023);
  deserializeJson(doc, payload, length);
  JsonVariant json_var = doc.as<JsonVariant>();
    
  DynamicJsonDocument telemetry(1023);
  telemetry.createNestedObject();
  
  const char* lampState = json_var[0]["payload"]["lampState"].as<char*>();
  const char* fanState = json_var[0]["payload"]["fanState"].as<char*>();
  const char* heaterState = json_var[0]["payload"]["heaterState"].as<char*>();
  const char* isAutomate = json_var[0]["payload"]["isAutomate"].as<char*>();
   
  if(strcmp(lampState,ON) == 0){
     digitalWrite(LAMP,HIGH);
     telemetry[0]["Lamp"] = 1;
     Serial.println("lamp roshan");
  }else if(strcmp(lampState,OFF) == 0){
     digitalWrite(LAMP,LOW);
     telemetry[0]["Lamp"] = 0;
     Serial.println("lamp khamush");
   }else{
     Serial.println(lampState);
   }
     if(strcmp(fanState,ON) == 0){
     digitalWrite(FAN,HIGH);
     telemetry[0]["Fan"] = 1;
     Serial.println("FAN roshan");
   }else if(strcmp(fanState,OFF) == 0){
     digitalWrite(FAN,LOW);
     telemetry[0]["Fan"] = 0;
     Serial.println("FAN khamush");
   }else{
     Serial.println(fanState);
   }
   if(strcmp(heaterState,ON) == 0){
    digitalWrite(HEATER,HIGH);
    telemetry[0]["Heater"] = 1;
    Serial.println("HEATER roshan");
   }else if(strcmp(heaterState,OFF) == 0){
     digitalWrite(HEATER,LOW);
     telemetry[0]["Heater"] = 0;
     Serial.println("HEATER khamush");
   }else{
     Serial.println(heaterState);
   }
    if(strcmp(isAutomate,ON) == 0){
     automate = true;
     telemetry[0]["Automate"] = 1;
     Serial.println("Automatic roshan");
   }else if(strcmp(isAutomate,OFF) == 0){
     automate = false;
     telemetry[0]["Automate"] = 0;
     Serial.println("Automatic khamush");
   }else{
     Serial.println(isAutomate);
   }
   
  DynamicJsonDocument commandResponse(1023);
  for (int i = 0; i < json_var.size(); i++) {
    unsigned int command_id = json_var[i]["id"].as<unsigned int>();
    commandResponse.createNestedObject();
    commandResponse[i]["id"] = command_id;
    commandResponse[i]["statusCode"] = 200;
    commandResponse[i]["payload"] = "done";
  }
  String telemetryTopic = "kp1/" + APP_VERSION + "/dcx/" + TOKEN + "/json";
  client.publish(telemetryTopic.c_str(), telemetry.as<String>().c_str());

  String responseTopic = "kp1/" + APP_VERSION + "/cex/" + TOKEN + "/result/SWITCH";
  client.publish(responseTopic.c_str(), commandResponse.as<String>().c_str());
  Serial.println("Published response to SWITCH command on topic: " + responseTopic);
}

void setup_wifi() {
  if (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.println();
    Serial.printf("Connecting to [%s]", ssid);
    WiFi.begin(ssid, password);
    connectWiFi();
  }
}

void connectWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    char *client_id = "client-id-123ab";
    if (client.connect(client_id)) {
      Serial.println("Connected to WiFi");
      // ... and resubscribe
      subscribeToCommand();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void subscribeToCommand() {
  String topic1 = "kp1/" + APP_VERSION + "/cex/" + TOKEN + "/command/SWITCH/status";
  client.subscribe(topic1.c_str());
  Serial.println("Subscribed on topic: " + topic1);
}

void setDeafaultState(){
    DynamicJsonDocument telemetry(1023);
    telemetry.createNestedObject();
    telemetry[0]["Lamp"] = 0;
    if(temp < 20.0){
        digitalWrite(HEATER,HIGH);
        telemetry[0]["Heater"] = 1;
       }
    if(temp > 25){
        digitalWrite(HEATER,LOW);
        telemetry[0]["Heater"] = 0;
    }
    if(temp > 32){
        digitalWrite(FAN,HIGH);
        telemetry[0]["Fan"] = 1;
    }
    if(temp < 30){
        digitalWrite(FAN,LOW);
        telemetry[0]["Fan"] = 0;
    }
    automate = true;
    telemetry[0]["Automate"] = 1;
       
    String topic = "kp1/" + APP_VERSION + "/dcx/" + TOKEN + "/json";
    client.publish(topic.c_str(), telemetry.as<String>().c_str());
  }