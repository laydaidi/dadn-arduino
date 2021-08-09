#include "esp_camera.h"
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <base64.h>

#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"
#include "camera_index.h"

const char* ssid =      "";        // Please change ssid & password
const char* password =  "";

#define AIO_SERVER      "io.adafruit.com"
#define AIO_PORT        1883
#define AIO_USERNAME    "CSE_BBC_phake"
#define AIO_KEY         ""
#define AIO_USERNAME1   "CSE_BBC_phake"
#define AIO_KEY1        ""

/************ Global State for MQTT client ******************/
WiFiClient      wifiClient;
WiFiClient      wifiClient1;
PubSubClient    mqttClient(wifiClient);
PubSubClient    mqttClient1(wifiClient1);

/****************************** Feeds ***************************************/
char subBtnTopic[] =    "CSE_BBC_phake/feeds/bk-iot-button";
char pubLEDTopic[] =    "CSE_BBC_phake/feeds/bk-iot-led";
char pubESPTopic[] =    "CSE_BBC_phake/feeds/bk-iot-esp32-cam";

/************ Global State for Web socket server ******************/
WebSocketsServer webSocket = WebSocketsServer(8888);
WiFiServer server(80);

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
        config.jpeg_quality = 30; //10-63 lower number means higher quality
        config.fb_count = 2;
    } 
    else {
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

void setup_websocket(){
    server.begin();
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    String IP = WiFi.localIP().toString();
    //String IP = WiFi.softAPIP()toString();
    Serial.println("WebSocket Server IP address: " + IP);
    Serial.println();
    index_html.replace("server_ip", IP);
}

void shutdown_websocket(){
    server.close();
    webSocket.close();
    Serial.println("WebSocket Server has been closed!");
    Serial.println();
}

void setup_wifi() { 
    IPAddress staticIP(192, 168, 1, 199);
    IPAddress gateway(192, 168, 1, 1); // = WiFi.gatewayIP();
    IPAddress subnet(255, 255, 255, 0); // = WiFi.subnetMask();
    IPAddress dns1(1, 1, 1, 1);
    IPAddress dns2(8, 8, 8, 8);

    if (!WiFi.config(staticIP, gateway, subnet, dns1, dns2)) {
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
void MQTT_connect() { 
    String clientId = "ESP32CAM"; 
    clientId += String(random(0xffff), HEX); 

    while (!mqttClient.connected()) { 
        Serial.print("Attempting MQTT connection..."); 

        if (mqttClient.connect(clientId.c_str(), AIO_USERNAME, AIO_KEY)) { 
            Serial.println("mqttClient connected"); 
            mqttClient.subscribe(subBtnTopic); 
        } else { 
            Serial.print("failed, rc="); 
            Serial.print(mqttClient.state()); 
            Serial.println(" try again in 2 seconds"); 
            delay(2000); 
        } 
    } 
}

void MQTT1_connect() { 
    String clientId = "ESP32CAM"; 
    clientId += String(random(0xffff), HEX); 

    while (!mqttClient1.connected()) { 
        Serial.print("Attempting MQTT1 connection..."); 

        if (mqttClient1.connect(clientId.c_str(), AIO_USERNAME1, AIO_KEY1)) { 
            Serial.println("mqttClient1 connected"); 
        } else { 
            Serial.print("failed, rc="); 
            Serial.print(mqttClient1.state()); 
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
    if (strcmp(topic, subBtnTopic) == 0){
        float value = doc["data"];

        if (value == 1.0){
            // Public ready message to esp32-topic
            String ready_msg = "{\"id\" : \"99\", \"name\" : \"ESP32\", \"data\": \"1\", \"unit\" : \"\"}";
            mqttClient1.publish(pubESPTopic, ready_msg.c_str()); 

            Serial.print("Message published [");
            Serial.print(pubESPTopic);
            Serial.print("] ");
            Serial.println(ready_msg);
            Serial.println();

            // Public led on message to LED-topic to notify LED
            String ledon_msg = "{\"id\" : \"1\", \"name\" : \"LED\", \"data\": \"2\", \"unit\" : \"\"}";
            mqttClient.publish(pubLEDTopic, ledon_msg.c_str()); 

            Serial.print("Message published [");
            Serial.print(pubLEDTopic);
            Serial.print("] ");
            Serial.println(ledon_msg);
            Serial.println();

            // Start web socket server
            setup_websocket();
        }
        else if (value == 0.0){
            // Public close message to esp32-topic
            String close_msg = "{\"id\" : \"99\", \"name\" : \"ESP32\", \"data\": \"0\", \"unit\" : \"\"}";
            mqttClient1.publish(pubESPTopic, close_msg.c_str()); 

            Serial.print("Message published [");
            Serial.print(pubESPTopic);
            Serial.print("] ");
            Serial.println(close_msg);
            Serial.println();

            // Public led off message to LED-topic to notify LED
            String ledoff_msg = "{\"id\" : \"1\", \"name\" : \"LED\", \"data\": \"0\", \"unit\" : \"\"}";
            mqttClient.publish(pubLEDTopic, ledoff_msg.c_str()); 

            Serial.print("Message published [");
            Serial.print(pubLEDTopic);
            Serial.print("] ");
            Serial.println(ledoff_msg);
            Serial.println();

            // Close web socket server
            shutdown_websocket();
        }
    }
}

void setup() {
    Serial.begin(115200);  

    setup_wifi();

    configCamera();

    mqttClient.setServer(AIO_SERVER, AIO_PORT);
    mqttClient1.setServer(AIO_SERVER, AIO_PORT);
    mqttClient.setCallback(callback);
    MQTT_connect();
    MQTT1_connect();
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

    mqttClient.loop(); 
    if (!mqttClient.connected()) { 
        MQTT_connect(); 
    }

    mqttClient1.loop(); 
    if (!mqttClient1.connected()) { 
        MQTT1_connect(); 
    }

    if(isClientConnected == true){
        //capture a frame
        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Frame buffer could not be acquired");
            return;
        }
        String fb_encoded = base64::encode((uint8_t *) fb->buf, fb->len);

        // Create JSON-String
        String strPayload = "{\"type\" : \"frame\", \"data\" : \"" + fb_encoded + "\" }";

        //Send serialized JSON to webSocket
        uint8_t* payload =  (uint8_t *)strPayload.c_str();
        webSocket.broadcastBIN(payload, strPayload.length()); // Broadcast can opened in many IPs

        //return the frame buffer back to be reused
        esp_camera_fb_return(fb);
    }
}
