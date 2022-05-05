#ifndef mwr_config_h
#define mwr_config_h

class MWConfig
{
  public:
    MWConfig();
    int detectMode(int source, int sink);
    void apMode();
    void stMode();
    String readUrlFromFile();
  private:
    void writeFile(fs::FS &fs, const char * path, const char * message);
    String readFile(fs::FS &fs, const char * path);
};

#endif
