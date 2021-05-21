#include "esp_camera.h"
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <base64.h>

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"
#include "camera_index.h"

#define MQTT_SERVER "169.55.61.243"
#define MQTT_PORT 1883
#define MQTT_USER "BBFF-aQZpnM5D7nUbgsbG3IBQOWiWAEILZB"
#define MQTT_PASSWORD ""
//#define MQTT_ESP32_TOPIC "NPNLab_BBC_phake/feeds/bk-iotesp"

char subTopic[] = "/v1.6/devices/bkiot/bk-iottouch";
char pubTopic[] = "/v1.6/devices/bkiot/bk-iotesp";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

WebSocketsServer webSocket = WebSocketsServer(8888);
WiFiServer server(80);
const char* ssid = "MANGDAYKTX H1-312";
const char* password = "12345789";

// Number of client connected
uint8_t client_num;
bool isClientConnected = false;

void configCamera(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Select lower framesize if the camera doesn't support PSRAM
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10; //10-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
          {
            Serial.printf("[client%u] Disconnected!", num);
            Serial.println();
            client_num = num;
            if(num == 0) isClientConnected = false;
          }
            break;
        case WStype_CONNECTED:
          {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[client%u] Connected from IP address: ", num);
            Serial.println(ip);
            client_num = num;
            isClientConnected = true;
          }
            break;
        case WStype_TEXT:
        case WStype_BIN:
        case WStype_ERROR:      
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            break;
    }
}

// Connnect to MQTT Server and subcribe topics
void connect_to_broker() { 
  while (!client.connected()) { 
    Serial.print("Attempting MQTT connection..."); 
    String clientId = "ESP32CAM"; 
    clientId += String(random(0xffff), HEX); 
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) { 
        Serial.println("connected"); 
        client.subscribe(subTopic); 
    } else { 
        Serial.print("failed, rc="); 
        Serial.print(client.state()); 
        Serial.println(" try again in 2 seconds"); 
        delay(2000); 
    } 
  } 
}

// Receive messages from subscribed topics and process
void callback(char* topic, byte *payload, unsigned int length) { 
  Serial.println();
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Deserialize the JSON document
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, (char *)payload);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  
  // Receive messages from button-topic
  if (strcmp(topic, subTopic) == 0){
    float value = doc["value"];
    
    if (value == 1.0){
      // Public ready message to esp32-topic
      String ready_msg = "{\"id\" : \"99\", \"name\" : \"ESP32\", \"value\": \"1\", \"unit\" : \"\"}";
      client.publish(pubTopic, ready_msg.c_str()); 
  
      Serial.print("Message published [");
      Serial.print(pubTopic);
      Serial.print("] ");
      Serial.println(ready_msg);
      Serial.println();

      // Start web socket server
      setup_websocket_server();
    }
    else {
      // Public ready message to esp32-topic
      String close_msg = "{\"id\" : \"99\", \"name\" : \"ESP32\", \"value\": \"0\", \"unit\" : \"\"}";
      client.publish(pubTopic, close_msg.c_str()); 
  
      Serial.print("Message published [");
      Serial.print(pubTopic);
      Serial.print("] ");
      Serial.println(close_msg);
      Serial.println();


      // Close web socket server
      shutdown_websocket_server();
    }
  }
}

void setup_websocket_server(){
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  String IP = WiFi.localIP().toString();
  //String IP = WiFi.softAPIP()toString();
  Serial.println("WebSocket Server IP address: " + IP);
  Serial.println();
  index_html.replace("server_ip", IP);
}

void shutdown_websocket_server(){
  server.close();
  webSocket.close();
  Serial.println("WebSocket Server has been closed!");
  Serial.println();
}

void setup_wifi() { 
  IPAddress staticIP(192, 168, 0, 199);
  IPAddress gateway(192, 168, 0, 1); // = WiFi.gatewayIP();
  IPAddress subnet(255, 255, 255, 0); // = WiFi.subnetMask();

  if (!WiFi.config(staticIP, gateway, subnet)) {
    Serial.print("Wifi configuration for static IP failed!");
  }

  WiFi.begin(ssid, password);
  //WiFi.softAP(ssid, password);  // Using wifi from access point in ESP32
  Serial.println("");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi " + String(ssid) + " connected.");
}

void setup() {
  Serial.begin(115200);  
  setup_wifi();
  configCamera();

  client.setServer(MQTT_SERVER, MQTT_PORT); 
  client.setCallback(callback); 
  connect_to_broker();
}
    
void http_resp(){
  WiFiClient client = server.available();
  if (client.connected() && client.available()) {                   
    client.flush();          
    client.print(index_html);
    client.stop();
  }
}

void loop() {
  http_resp();
  webSocket.loop();

  client.loop(); 
  if (!client.connected()) { 
    connect_to_broker(); 
  }
  
  if(isClientConnected == true){
    //capture a frame
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Frame buffer could not be acquired");
        return;
    }
    String fb_encoded = base64::encode((uint8_t *) fb->buf, fb->len);
//    String fb_encoded = (uint8_t *) fb->buf;

    // Create JSON-String
    String strPayload = "{\"type\" : \"frame\", \"data\" : \"" + fb_encoded + "\" }";

    //Send serialized JSON to webSocket
    uint8_t* payload =  (uint8_t *)strPayload.c_str();
    webSocket.broadcastBIN(payload, strPayload.length()); // Broadcast can opened in many IPs

    //return the frame buffer back to be reused
    esp_camera_fb_return(fb);
  }
}
