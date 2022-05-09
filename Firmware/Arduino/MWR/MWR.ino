#include <Arduino.h>
#include <SPIFFS.h>
#include "mwr_radio.h";
#include "mwr_config.h";

// Pin Definitions
#define I2S_DOUT 26
#define I2S_BCLK 25
#define I2S_LRC  33
#define I2S_GAIN 27
#define I2S_SD 14
#define LED_BATT 5
#define LED_WIFI 18
#define VOL 34
#define BATT 12
#define MODE_0 13
#define MODE_1 15

// Initalise Libaries
MWConfig mConfig;
MWRadio mRadio;

// Initialise Battery Task
TaskHandle_t ChckBatTsk;

int mode = 0;   // Mode State

void setup() {
  // Start Serial for debugging
  Serial.begin(115200);
  pinMode(LED_BATT,OUTPUT);
  pinMode(LED_WIFI,OUTPUT);

  // Start SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");

  // Create Battery Task
  xTaskCreatePinnedToCore(CheckBattery,"ChckBatTsk",10000,NULL,0,&ChckBatTsk,0);

  // Detect operating mode
  mode = mConfig.detectMode(MODE_0,MODE_1);
  if(!mode){   
    // Start into Config mode
    mConfig.apMode();
    while (WiFi.status() != WL_CONNECTED){flashLED(LED_WIFI,150);delay(500);}
  }else{      
    // Start into Radio mode
    mConfig.stMode();
    while (WiFi.status() != WL_CONNECTED){flashLED(LED_WIFI,150);delay(500);}
    WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
    // Initalise Audio
    mRadio.init(I2S_DOUT, I2S_BCLK, I2S_LRC, I2S_GAIN, I2S_SD);
    // Get and Set Station URL
    String url = mConfig.readUrlFromFile();
    mRadio.setStation(url);
  }
}

void loop() {
  if(mode){
   mRadio.setVolume(getVolume());
   mRadio.play();   
  }
}

// Get volume state, mapped 1-20
int getVolume()
{
  return map(analogRead(VOL), 0, 4095, 1, 20);
}

// Check Battery Level
void CheckBattery( void * parameter) {
  float voltage = 0;
  int timer = 200;
  for(;;) {
    if(timer >= 200){ voltage = ((float)analogRead(35) / 4095) * 3.3 * 2 * 1.035; timer = 0; }
      if(voltage > 4) {
        digitalWrite(LED_BATT,HIGH); delay(100); digitalWrite(LED_BATT,LOW); delay(100);
        digitalWrite(LED_BATT,HIGH); delay(300); digitalWrite(LED_BATT,LOW); delay(100);
      }else{
        if(voltage > 3.7){
          digitalWrite(LED_BATT,HIGH);    // Normal Range
          delay(1000);
        }else{
          if(voltage > 3.5){              // Lower End
            flashLED(LED_BATT,500);
          }else{                   
            flashLED(LED_BATT,150);       // Low or Anonomly
          }
        }
      }
    timer++;
    Serial.print("Battery:");
    Serial.println(voltage);
    Serial.print("RSSI:");
    Serial.println(WiFi.RSSI());
  }
}

// Flash an LED
void flashLED(int LED, int SPEED)
{
  digitalWrite(LED,HIGH);
  delay(SPEED);
  digitalWrite(LED, LOW);
  delay(SPEED);
}


void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.reconnect();
}
