/*
 * Created by Jakub Svajka on 2021-02-11.
 * 
 * Based on SerialESP8266wifi.h by Jonas Ekstrand on 2015-02-20.
 * ESP8266 AT commands reference: https://github.com/espressif/esp8266_at/wiki
 */
#ifndef ESP8266_WLAN_H
#define ESP8266_WLAN_H

#include "Arduino.h"
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

#define MAX_BUFFER_SIZE 512
#define MAX_CONNECTIONS 3
#define MAX_RESET_ATTEMPTS 3

typedef unsigned char byte;


struct WifiMessage {
public:
    WifiMessage() {
        this->overflowed = false;
        this->hasData = false;
        this->channel = '-';
        this->message = NULL;
    };
    bool overflowed:1;
    bool hasData:1;
    char channel;
    char * message;
};

struct WifiConnection {
public:
    char channel;
    bool connected:1;
};

struct Flags {
    bool initialized:1,
         connectedToAP:1,
         tcpServerRunning:1,
         sending:1,
         unexpectedEcho:1;
};

class ESP8266_WLAN : public ESP8266_AT
{
public:
    ESP8266_WLAN(byte RX_PIN, byte TX_PIN, byte RST_PIN);
    ~ESP8266_WLAN();

    bool isActive();
    bool init();

    bool connectToAP(const char * ssid, const char * pass);
    bool disconnectFromAP();

    char * getIP();
    char * getMAC();
    char getStatus();
    bool closeConnection(char channel);

    bool createTCPServer(const char * port);
    bool deleteTCPServer();

    bool send(char channel, String& message, bool eol = true, bool sendNow = true);
    bool send(char channel, const char * message, bool eol = true, bool sendNow = true);
    bool send_PROGMEM(char channel, const char * message);

    byte update();
    WifiMessage * getWifiMessage();

    byte readData();
    byte readData(size_t origin);
    byte checkResponse();

    void writeCommand(const char * cmd, bool eol = true);
    bool readCommand(const char * cmd, bool progmem = true);

    size_t readLine(char * buf, size_t len);

    char BUFFER[MAX_BUFFER_SIZE];
    size_t CUR_BUFFER_SIZE;
protected:
    WifiMessage msg;
private:
    byte _RST_PIN;

    Flags _flags;

    bool createTCPServer();
    char _ip[16];
    char _mac[18];
    char _port[6];

    bool connectToAP();
    bool isConnectedToAP();
    char _ssid[16];
    char _pass[16];

    bool anyClientConnected();

    bool diagnose();
    bool restart();
    bool softRestart();
    bool hardRestart();

    void updateWifiMessage();
    WifiConnection _connections[MAX_CONNECTIONS];
};


#endif
