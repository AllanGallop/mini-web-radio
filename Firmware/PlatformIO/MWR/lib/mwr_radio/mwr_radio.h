#ifndef mwr_radio_h
#define mwr_radio_h

#include "Audio.h"

class MWRadio
{
  public:
    void init(int DOUT, int BLCK, int LRC, int GAIN, int SDP);
    void setStation(String URL);
    void setVolume(int VOL);
    void play();

};

#endif
