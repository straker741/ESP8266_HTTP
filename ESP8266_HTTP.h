/*
 * Created by Jakub Svajka on 2021-02-15.
 */
#ifndef ESP8266_HTTP_H
#define ESP8266_HTTP_H

//#include "Arduino.h"
#include "ESP8266_WLAN.h"
#include <avr/pgmspace.h>

#define MAX_ROUTES 3


enum HTTP_Method { GET, HEAD, POST, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH, HTTP_METHOD_LENGTH };


class Route
{
public:
    Route();
    Route(HTTP_Method method, const char * path);
    ~Route();

    void set(byte ID, HTTP_Method method, const char * path);
    void setParams(const char * params);
    bool operator==(const Route & route);

    byte getID() { return _id; }
    HTTP_Method getMethod() {return _method; }
    char * getPath() { return _path; }
    char * getParams() { return _params; }
private:
    byte _id;
    HTTP_Method _method;
    char * _path;
    char * _params;
};


class Router
{
public:
    Router();
    ~Router();

    void registerRoute(HTTP_Method method, const char * path);
    Route * isRegistered(const char * method, const char * path);
    Route * isRegistered(HTTP_Method method, const char * path);

    size_t size() { return _size; }
private:
    Route _routes[MAX_ROUTES];
    size_t _size;
};


class ESP8266_HTTP : public ESP8266_WLAN, public Router
{
public:
    ESP8266_HTTP(byte RX_PIN, byte TX_PIN, byte RST_PIN);
    ~ESP8266_HTTP();

    byte start(const char * ssid, const char * pass, const char * port);
    bool isHTTP();
    bool isHTTP(const char * message);

    Route * preprocessRequest();

    void send404();
    void send200();
};


#endif
