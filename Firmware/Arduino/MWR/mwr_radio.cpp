#include <Arduino.h>
#include "Audio.h"
#include "mwr_radio.h"

Audio audio;

void MWRadio::init(int DOUT, int BLCK, int LRC, int GAIN, int SDP)
{
  pinMode(GAIN,OUTPUT);
  pinMode(SDP, OUTPUT);
  digitalWrite(GAIN,HIGH);
  digitalWrite(SDP,HIGH);
  audio.setPinout(BLCK, LRC, DOUT);
}
void MWRadio::setStation(String URL)
{
  audio.connecttohost(URL.c_str());
}
void MWRadio::setVolume(int VOL)
{
  audio.setVolume(VOL);
}
void MWRadio::play()
{
  audio.loop();
}
