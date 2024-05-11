#include "WiFi.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>


#define REDPIN 3
#define GREENPIN 4
#define BLUEPIN 5
#define COLD_WHITE 18
#define WARM_WHITE 19
#define FADESPEED 20     // make this higher to slow down
#define ID_Device "123666888"

#define ssid "Administrator’s iPhone"
#define password    "30042002"

const char* mqtt_server = "mqttvcloud.innoway.vn";
const char* mqttUser = "minhhk1";
const char* mqttPassword = "KXXlVA14vyvy5wbuMR5MOVOUPUUfSWxh";
WiFiClient espClient;
PubSubClient client(espClient);

int control_white = 0;
int control_rgb = 0;


void taskBlinkWhite(void *pvParameters) {
  for (;;) {
    if (control_white == 0) {
      analogWrite(WARM_WHITE, 256);
      vTaskDelay(pdMS_TO_TICKS(500)); // Thay thế delay(1000) bằng vTaskDelay
      analogWrite(WARM_WHITE, 0);
      vTaskDelay(pdMS_TO_TICKS(500)); // Thay thế delay(1000) bằng vTaskDelay
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void control_rgb_2(int red, int blue, int green){
  analogWrite(REDPIN, red);
  analogWrite(GREENPIN, blue);
  analogWrite(BLUEPIN, green);
}

void hanldeRGB(String messageTemp){
  int colonIndex = messageTemp.indexOf(':');
  String numbersString = messageTemp.substring(colonIndex + 1);
  int numbers[3];
  int count = 0;
  int commaIndex = 0;
  do {
      commaIndex = numbersString.indexOf(',');
      String numberString = numbersString.substring(0, commaIndex);
      numbers[count] = numberString.toInt();
      numbersString = numbersString.substring(commaIndex + 1);
      count++;
  } while (commaIndex != -1 && count < 3);
  control_rgb_2(numbers[0],numbers[1],numbers[2]);
  for (int i = 0; i < count; i++) {
      Serial.print("Number ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.println(numbers[i]);

  }
}

void taskBlinkRGB(void *pvParameters) {
  for (;;) {
    int r, g, b;
      if(control_rgb == 0){
        for (r = 0; r < 256; r++) {
            if(control_rgb == 1){
              break;
            }
            analogWrite(REDPIN, r);
            vTaskDelay(pdMS_TO_TICKS(FADESPEED));
        }
      // fade from violet to red
        for (b = 255; b > 0; b--) {
            if(control_rgb == 1){
              break;
            }
            analogWrite(BLUEPIN, b);
            vTaskDelay(pdMS_TO_TICKS(FADESPEED));
        }
      // fade from red to yellow
        for (g = 0; g < 256; g++) {
            if(control_rgb == 1){
              break;
            }
            analogWrite(GREENPIN, g);
            vTaskDelay(pdMS_TO_TICKS(FADESPEED));
        }
      // fade from yellow to green
        for (r = 255; r > 0; r--) {
            if(control_rgb == 1){
              break;
            }
            analogWrite(REDPIN, r);
            vTaskDelay(pdMS_TO_TICKS(FADESPEED));
        }
      // fade from green to teal
        for (b = 0; b < 256; b++) {
            if(control_rgb == 1){
              break;
            }
            analogWrite(BLUEPIN, b);
            vTaskDelay(pdMS_TO_TICKS(FADESPEED));
        }
      // fade from teal to blue
        for (g = 255; g > 0; g--) {
            if(control_rgb == 1){
              break;
            }
            analogWrite(GREENPIN, g);
            vTaskDelay(pdMS_TO_TICKS(FADESPEED));
        }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  // Feel free to add more if statements to control more GPIOs with MQTT
  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "message/control/white") {
    Serial.print("Changing output to ");
    if (messageTemp.indexOf("on") != -1){
      control_white = 1;
      Serial.println("on white");
      analogWrite(WARM_WHITE, 256);
      send_message_mqtt("white","on");
    }
    else if(messageTemp.indexOf("off") != -1){
      control_white = 1;
      Serial.println("off while");
      analogWrite(WARM_WHITE, 0);
      send_message_mqtt("white","off");
    }
    else if(messageTemp.indexOf("blink") != -1){
      control_white = 0;
      Serial.println("blink white");
      send_message_mqtt("white","blink");
    }
    else if(messageTemp.indexOf("dim")!= -1){       //thay doi do sang
        int idx = messageTemp.indexOf(":");
        String dim = messageTemp.substring(idx);
        Serial.println("dim:50");
        analogWrite(WARM_WHITE, dim.toInt());
    }
  }
  else if(String(topic) == "message/control/rgb"){
      Serial.print("Changing output RGB");
      if (messageTemp.indexOf("on") != -1){
        control_rgb = 0;
        Serial.println("on rgb");
        send_message_mqtt("rgb","on");
      }
      else if(messageTemp.indexOf("off") != -1){
        control_rgb = 1;
        Serial.println("off rgb");
        send_message_mqtt("rgb","off");
        control_rgb_2(0,0,0);
      }
      else if(messageTemp.indexOf("auto") != -1){
        control_rgb = 0;
        send_message_mqtt("rgb","on");
        Serial.println("auto rgb");
      }
      else if(messageTemp.indexOf("manual") != -1){
        control_rgb = 1;
        send_message_mqtt("rgb","on");
        Serial.println("manual rgb");
        hanldeRGB(messageTemp);
      }
  }
  Serial.println("Exit Call back");
}


void setup() {
  Serial.begin(115200);
  pinMode(COLD_WHITE, OUTPUT);
  analogWrite(COLD_WHITE, 256);
  //Init WiFi as Station, start SmartConfig
  // WiFi.mode(WIFI_AP_STA);
  // WiFi.beginSmartConfig();
  WiFi.begin(ssid, password);
  

  //Wait for SmartConfig packet from mobile
  // Serial.println("Waiting for SmartConfig.");
  // while (!WiFi.smartConfigDone()) {
  //   vTaskDelay(pdMS_TO_TICKS(500));
  //   Serial.print(".");
  // }

  Serial.println("");
  Serial.println("SmartConfig received.");

  //Wait for WiFi to connect to AP
  Serial.println("Waiting for WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
  }

  Serial.println("WiFi Connected.");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
  pinMode(WARM_WHITE, OUTPUT);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  xTaskCreate(taskBlinkWhite, "BlinkWhite", 1000, NULL, 1, NULL);
  xTaskCreate(taskBlinkRGB, "BlinkRGB", 1000, NULL, 1, NULL);
} 

void send_message_mqtt(String type, String status){
    StaticJsonDocument<100> jsonDocument;
    jsonDocument["id"] = ID_Device;
    jsonDocument["status"] = status;
    jsonDocument["type"] = type;
    char buffer[100];
    serializeJson(jsonDocument, buffer);
    client.publish("message/status/rgb", buffer);
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("cien", mqttUser, mqttPassword)) {
      Serial.println("check");
      analogWrite(COLD_WHITE, 0);
      
      control_rgb = 0;
      Serial.println("connected to broker");
      client.subscribe("message/control/white");
      client.subscribe("message/control/rgb");
      client.subscribe("message/response/rgb");
      send_message_mqtt("white","on");
      send_message_mqtt("rgb","on");
    } else {
      control_white = 1;
      if(control_rgb == 0){
        control_rgb = 1;
        control_rgb_2(0,0,0);
      }
      analogWrite(WARM_WHITE, 0);
      analogWrite(COLD_WHITE, 256);
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      vTaskDelay(pdMS_TO_TICKS(FADESPEED));
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  vTaskDelay(pdMS_TO_TICKS(5));
}