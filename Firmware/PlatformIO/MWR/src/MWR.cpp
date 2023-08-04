/**
 * Mini-Web Radio V1.3 - No Battery
 * ======================
 * Created by Allan Gallop
 * For Milton Keynes Hospital Radio
 */

#include <Arduino.h>
#include <SPIFFS.h>
#include "driver/rtc_io.h"
#include "mwr_radio.h";
#include "mwr_config.h";

// Pin Definitions
#define I2S_DOUT 27
#define I2S_BCLK 26
#define I2S_LRC  25
#define I2S_GAIN 14
#define I2S_SD 12
#define LED_WIFI 18
#define VOL 34
#define MODE_0 13
#define MODE_1 15
// if you change this make sure to use an RTC pin and change it on lines 41,43 too
#define POWER_SW 33

// Initalise Libaries
MWConfig mConfig;
MWRadio mRadio;

int mode = 0;   // Mode State

void setup() {
  // Start Serial for debugging
  Serial.begin(115200);

  // Setup LED pins
  pinMode(LED_WIFI,OUTPUT);

  // Configure wakeup source for the power switch
  // First we enable the rtc pulldown
  rtc_gpio_pulldown_en(GPIO_NUM_33);
  // then setup GPIO 34 as the wakeup source
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33,1);

  // Start SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");

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
    powerSense();
    mRadio.setVolume(getVolume());
    mRadio.play();
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
 *  powerSense
 *  --------------------------
 *  Check the state of the power switch and put the ESP to sleep
 *  if input is low (switch open)
*/
void powerSense()
{
   if(digitalRead(POWER_SW) === LOW)
   {
      esp_deep_sleep_start();
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
