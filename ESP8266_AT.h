/*
 * Created by Jakub Svajka on 2021-02-11.
 * 
 * Based on SerialESP8266wifi.h by Jonas Ekstrand on 2015-02-20.
 * ESP8266 AT commands reference: https://github.com/espressif/esp8266_at/wiki
 */
#ifndef ESP8266_AT_H
#define ESP8266_AT_H

#include "Arduino.h"
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

#define MAX_BUFFER_SIZE 512

typedef unsigned char byte;


// Class responsible for serial communication with AT comands
class ESP8266_AT : public SoftwareSerial
{
public:
    ESP8266_AT(byte RX_PIN, byte TX_PIN);
    ~ESP8266_AT();

    byte readData();
    byte readData(size_t origin);
    byte checkResponse();

    void writeCommand(const char * cmd, bool eol = true);
    bool readCommand(const char * cmd, bool progmem = true);

    size_t readLine(char * buf, size_t len);

    char BUFFER[MAX_BUFFER_SIZE];
    size_t CUR_BUFFER_SIZE;
};


#endif
