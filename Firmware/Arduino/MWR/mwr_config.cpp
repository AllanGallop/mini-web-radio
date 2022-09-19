#include <Arduino.h>
#include "FS.h"
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "mwr_config.h"

const char* PARAM_INPUT_1 = "host";
const char* PARAM_INPUT_2 = "ssid";
const char* PARAM_INPUT_3 = "pass";
const char* PARAM_INPUT_4 = "url";

String host;
String ssid;
String pass;
String url;

const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* hostPath = "/host.txt";
const char* urlPath = "/url.txt";

IPAddress localIP(192,168,1,1);
IPAddress localGateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

AsyncWebServer server(80);

MWConfig::MWConfig()
{}

int MWConfig::detectMode(int source, int sink)
{
  /**
   * v1.0 hardware only
   * --------------------
   * We use two pins to denote state via short
   */
  pinMode(source,OUTPUT);
  pinMode(sink,INPUT_PULLUP);
  delay(1);
  digitalWrite(source,LOW);
  delay(1);

  // Read mode
  int mode = digitalRead(sink);
  Serial.println(mode);
  delay(1);
  digitalWrite(source,LOW);
  if(!mode){ Serial.println("Configuration Jumper Detected"); }
  return mode;
}

void MWConfig::apMode()
{
  Serial.println("Starting WiFi in AP Mode");
  WiFi.disconnect();
  WiFi.softAPConfig(localIP,localGateway,subnet);
  WiFi.softAP("MWR-WIFI-SETUP", NULL);

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      Serial.println("Inbound Request");
      request->send(SPIFFS, "/index.html", "text/html");
    });
    
    server.serveStatic("/", SPIFFS, "/");
    
    server.on("/", HTTP_POST, [this](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          if (p->name() == PARAM_INPUT_1) {
            host = p->value().c_str();
            writeFile(SPIFFS, hostPath, host.c_str());
          }
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_2) {
            ssid = p->value().c_str();
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_3) {
            pass = p->value().c_str();
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            url = p->value().c_str();
            // Write file to save value
            writeFile(SPIFFS, urlPath, url.c_str());
          }
        }
      }
      request->send(200, "text/plain", "Done. "+host+" will restart");
      delay(3000);
      ESP.restart();
    });
    server.begin();
}

void MWConfig::stMode()
{
  Serial.println("Starting WiFi in Station Mode");
  host = readFile(SPIFFS, hostPath);
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(host.c_str());
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  if(pass == NULL| pass.isEmpty()){
    Serial.println("with no password");
    WiFi.begin(ssid.c_str());
  }else{
    WiFi.begin(ssid.c_str(), pass.c_str());
  }
}

String MWConfig::readUrlFromFile()
{
  return readFile(SPIFFS, urlPath);
}

// Read File from SPIFFS
String MWConfig::readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS 
void MWConfig::writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}
