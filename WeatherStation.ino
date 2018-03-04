/*
This example will open a configuration portal when no WiFi configuration has been previously
entered or when a button is pushed. It is the easiest scenario for configuration but 
requires a pin and a button on the ESP8266 device. The Flash button is convenient 
for this on NodeMCU devices.
*/

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>  
#include <PubSubClient.h>
#include <DHT.h>
#define DHTTYPE DHT11
#define DHTPIN  D5


DHT dht(DHTPIN, DHTTYPE, 11);
ESP8266WebServer server(80);


float temperature, humidity;
unsigned long previousMillis = 0;
const long interval = 2000;

/****************************************
 * Define Constants
 ****************************************/
 
namespace{
  const char * AP_NAME = "Weather_station"; // My Access Point name
  const char * MQTT_SERVER = "things.ubidots.com"; 
  const char * TOKEN = "ubidots_token(Put your)"; // My Ubidots TOKEN
  const char * DEVICE_LABEL = "ESP8266"; // My Device Label
  const char * VARIABLE_LABEL1 = "humidity"; // My Variable Label
  const char * VARIABLE_LABEL2 = "temperature";
  int SENSOR = D4;
}

char topic[150];
char payload[50];
String clientMac = "";
unsigned char mac[6];


/****************************************
 * Initialize a global instance
 ****************************************/
WiFiClient espClient;
PubSubClient client(espClient);


//https://github.com/kentaylor/WiFiManager
// select wich pin will trigger the configuraton portal when set to LOW
// ESP-01 users please note: the only pins available (0 and 2), are shared 
// with the bootloader, so always set them HIGH at power-up
// Onboard PIN_LED I/O pin on NodeMCU board
const int PIN_LED = 2; // D4 on NodeMCU and WeMos. Controls the onboard PIN_LED.
/*Trigger for inititating config mode is Pin D3 and also flash button on NodeMCU
 * Flash button is convenient to use but if it is pressed it will stuff up the serial port device driver 
 * until the computer is rebooted on windows machines.
 */
const int TRIGGER_PIN = 0; // D3 on NodeMCU and WeMos.
/*
 * Alternative trigger pin. Needs to be connected to a button to use this pin. It must be a momentary connection
 * not connected permanently to ground. Either trigger pin will work.
 */
const int TRIGGER_PIN2 = 13; // D7 on NodeMCU and WeMos.

// Indicates whether ESP has WiFi credentials saved from previous session
bool initialConfig = false;

/****************************************
 * Auxiliar Functions
 ****************************************/

void callback(char* topic, byte* payload, unsigned int length){
  
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientMac.c_str(), TOKEN, NULL)) {
      Serial.println("connected");
      break;
      } else {
        ESP.reset();
        Serial.print("faiPIN_LED, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 3 seconds");
        for(uint8_t Blink=0; Blink<=3; Blink++){
          digitalWrite(PIN_LED, LOW);
          delay(500);
          digitalWrite(PIN_LED, HIGH);
          delay(500);
        }
      }
  }
}

String macToStr(const uint8_t* mac) {
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)result += ':';
  }
  return result;
}


void setup() {
  // put your setup code here, to run once:
 pinMode(SENSOR,INPUT);
 
 /* Assign WiFi MAC address as MQTT client name */
  WiFi.macAddress(mac);
  clientMac += macToStr(mac);

   /* Set Sets the server details */
  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);

  /* Build the topic request */
  sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  
  // initialize the PIN_LED digital pin as an output.
  pinMode(PIN_LED, OUTPUT);
  Serial.begin(115200);
  dht.begin();
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  server.on("/dht11", HTTP_GET, [](){
        unsigned long currentMillis = millis();

        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;

            humidity = dht.readHumidity();
            temperature = dht.readTemperature();
            if (std::isnan(humidity) || std::isnan(temperature)) {
                Serial.println("Failed to read from DHT sensor!");
                return;
            }
        }

        String webString = "Humiditiy " + String((int)humidity) + "%   Temperature: " + String((int)temperature) + " C";
        Serial.println(webString);
        server.send(200, "text/plain", webString);
    });
    server.on("/dht11.json", [](){
        unsigned long currentMillis = millis();

        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;

            humidity = dht.readHumidity();
            temperature = dht.readTemperature();

            if (std::isnan(humidity) || std::isnan(temperature)) {
                Serial.println("Failed to read from DHT sensor!");
                return;
            }

            Serial.println("Reporting " + String((int)temperature) + "C and " + String((int)humidity) + " % humidity");
        }

        StaticJsonBuffer<500> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        root["temperature"] = temperature;
        root["humidity"] = humidity;

        String jsonString;
        root.printTo(jsonString);

        Serial.println(jsonString);
        server.send(200, "application/json", jsonString);
    });

    server.begin();
    Serial.println("HTTP server started! Waiting for clients!");
  
  
  Serial.println("\n Starting");
  WiFi.printDiag(Serial); //Remove this line if you do not want to see WiFi password printed
  if (WiFi.SSID()==""){
    Serial.println("We haven't got any access point credentials, so get them now");   
    initialConfig = true;
  }
  else{
    digitalWrite(PIN_LED, HIGH); // Turn PIN_LED off as we are not in configuration mode.
    WiFi.mode(WIFI_STA); // Force to station mode because if device was switched off while in access point mode it will start up next time in access point mode.
    unsigned long startedAt = millis();
    Serial.print("After waiting ");
    int connRes = WiFi.waitForConnectResult();
    float waited = (millis()- startedAt);
    Serial.print(waited/1000);
    Serial.print(" secs in setup() connection result is ");
    Serial.println(connRes);
  }
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  pinMode(TRIGGER_PIN2, INPUT_PULLUP);
  if (WiFi.status()!=WL_CONNECTED){
    Serial.println("faiPIN_LED to connect, finishing setup anyway");
  } else{
    Serial.print("local ip: ");
    Serial.println(WiFi.localIP());
  }
  

}

void loop() {
  // is configuration portal requested?
  if ((digitalRead(TRIGGER_PIN) == LOW) || (digitalRead(TRIGGER_PIN2) == LOW) || (initialConfig)) {
     Serial.println("Configuration portal requested.");
     digitalWrite(PIN_LED, LOW); // turn the PIN_LED on by making the voltage LOW to tell us we are in configuration mode.
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    //sets timeout in seconds until configuration portal gets turned off.
    //If not specified device will remain in configuration mode until
    //switched off via webserver or device is restarted.
    //wifiManager.setConfigPortalTimeout(600);

    //it starts an access point 
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.startConfigPortal()) {
      Serial.println("Not connected to WiFi but continuing anyway.");
    } else {
      //if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");
    }
    digitalWrite(PIN_LED, HIGH); // Turn PIN_LED off as we are not in configuration mode.
    ESP.reset(); // This is a bit crude. For some unknown reason webserver can only be started once per boot up 
    // so resetting the device allows to go back into config mode again when it reboots.
    delay(5000);
  }


  // put your main code here, to run repeatedly:
  /* MQTT client reconnection */
  if (!client.connected()) {
      reconnect();
  }
  
  server.handleClient();
 
  /* Sensor Reading */
  //int value = digitalRead(SENSOR);
  int value1=humidity;
  int value2=temperature;
  /* Build the payload request */
 sprintf(payload, "{\"%s\": %d}", VARIABLE_LABEL1, value1);
  /* Publish sensor value to Ubidots */ 
  client.publish(topic, payload);
  client.loop();

   /* Build the payload request */
 sprintf(payload, "{\"%s\": %d}", VARIABLE_LABEL2, value2);
  /* Publish sensor value to Ubidots */ 
  client.publish(topic, payload);
  client.loop();
  delay(1000);

}
