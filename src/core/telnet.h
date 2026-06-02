#ifndef telnet_h
#define telnet_h
#include <WiFi.h>

#define MAX_TLN_CLIENTS 5
#define BOOTLOG( ... ) { char buf[120]; sprintf( buf, __VA_ARGS__ ) ; telnet.printf("##[BOOT]#\t%s\n",buf); }

class Telnet {
  public:
    Telnet() {};
    bool begin(bool quiet=false);
    void loop();
    void start();
    void stop();
    void toggle();
    void print(uint8_t id, const char *buf);
    void print(const char *buf);
    void printf(uint8_t id, const char *format, ...);
    void printf(const char *format, ...);
    void cleanupClients();
    void info();
  protected:
    WiFiServer server = WiFiServer(23);
    WiFiClient clients[MAX_TLN_CLIENTS];
    void emptyClientStream(WiFiClient client);
    void on_connect(const char* str, uint8_t clientId);
    void on_input(const char* str, uint8_t clientId);
  private:
    char cmBuf[220];
    char lastMetaLine[220] = {};
    int  lastVolValue = -1;
    uint32_t lastMetaMs = 0;
    uint32_t lastVolMs = 0;
    char lastInfoSig[220] = {};
    uint32_t lastInfoMs = 0;
    bool _isIPSet(IPAddress ip);
    bool _shouldSuppressCliLine(const char *line);
    bool _shouldSuppressInfoSnapshot();
    void handleSerial();
    void printHeapFragmentationInfo(uint8_t id);
};

extern Telnet telnet;

#endif
