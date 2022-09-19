/**
 * Mini-Web Radio V1.3
 * ======================
 * Created by Allan Gallop
 * For Milton Keynes Hospital Radio
 */

#include <Arduino.h>
#include <SPIFFS.h>
#include "mwr_radio.h";
#include "mwr_config.h";

// Pin Definitions
#define I2S_DOUT 27
#define I2S_BCLK 26
#define I2S_LRC  25
#define I2S_GAIN 14
#define I2S_SD 12
#define LED_BATT 5
#define LED_WIFI 18
#define VOL 34
#define MODE_0 13
#define MODE_1 15
int VBATT = 33;

// Initalise Libaries
MWConfig mConfig;
MWRadio mRadio;

// Initialise Battery Task
TaskHandle_t ChckBatTsk;

int mode = 0;   // Mode State

void setup() {
  // Start Serial for debugging
  Serial.begin(115200);
  // Setup LED pins
  pinMode(LED_BATT,OUTPUT);
  pinMode(LED_WIFI,OUTPUT);

  // Start SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");

  // Create Battery Task on other core
  xTaskCreatePinnedToCore(CheckBattery,"ChckBatTsk",10000,(void*)&VBATT,0,&ChckBatTsk,NULL);

  // Detect operating mode
  mode = mConfig.detectMode(MODE_0,MODE_1);
  if(!mode){   
    // Start into Config mode
    mConfig.apMode();
    // Flash wifi led until connected
    while (WiFi.status() != WL_CONNECTED){flashLED(LED_WIFI,150);delay(500);}
  }else{      
    // Start into Radio mode
    mConfig.stMode();
    // Flash wifi led until connected
    while (WiFi.status() != WL_CONNECTED){flashLED(LED_WIFI,150);delay(500);}
    // For Arduino 2.x / ESP 2.x replace below line with:
    // WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
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
    /** 
     *  V1.0 Hardware only (Hack)
     *  ----------------------
     *  As battery is on mechanical switch we treat minimum volume
     *  as effectly a "charge only" mode
     */
    if(getVolume() >1){
     mRadio.setVolume(getVolume());
     mRadio.play();
    }
  }
}

/**
 *   getVolume
 *   -------------------------
 *   Returns int volume within range 1-20
 */
int getVolume()
{
  return map(analogRead(VOL), 0, 4095, 1, 20);
}

/**
 *  CheckBattery
 *  -------------------------
 *  Callback task for alternate core, monitors battery state
 */
void CheckBattery( void * _VBATT) {
  int timer = 200;
  float voltage = 0.0;
  // psuedo loop
  for(;;) {
    // check every 200 ticks
    if(timer >= 200){
      // Read voltage
      voltage = ((float)analogRead(*((int*)_VBATT)) / 4095) * 3.3 * 2 * 1.035;
      // Dump to serial
      Serial.print("Battery:");
      Serial.println(voltage);
      Serial.print("RSSI:");
      Serial.println(WiFi.RSSI());
      // Battery monitoring
      if(voltage > 4) {                   // Typical Charging voltage
          flashLED(LED_BATT,500);flashLED(LED_BATT,250);
      }else{
        if(voltage > 3.7){
          digitalWrite(LED_BATT,HIGH);    // Normal Range
        }else{
          if(voltage > 3.5){              // Lower End
            flashLED(LED_BATT,500);
          }else{                   
            flashLED(LED_BATT,150);       // Low
            if(voltage <= 3.0){
              flashLED(LED_BATT,50);      // Danger, sleep ESP
              Serial.println("Battery low, going to sleep");
              delay(1000);
              Serial.flush(); 
              esp_deep_sleep_start();
            }
          }
        }
      }
    timer++;
   }
  }
}

/**
 *  flashLED
 *  -------------------------
 *  Alternates LED state with delay
 */
void flashLED(int LED, int SPEED)
{
  digitalWrite(LED,HIGH);
  delay(SPEED);
  digitalWrite(LED, LOW);
  delay(SPEED);
}

/**
 *  WifiStationDisconnected
 *  -------------------------
 *  Handles disconnection event, attempts to reconnect
 */
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.reconnect();
}
