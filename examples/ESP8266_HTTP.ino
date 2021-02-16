/*
 * Created by Jakub Svajka on 2021-02-15.
 */
#include "ESP8266_HTTP.h"
#include <avr/pgmspace.h>

#define RX_PIN  4  // Connect this pin to TX on the esp8266
#define TX_PIN  6  // Connect this pin to RX on the esp8266
#define RST_PIN 5

#define SSID "ssid1234"
#define PASS "pass1234"
#define PORT "80"


void processRequest(Route * route);

// Custom HTTP response
const char PROGMEM_HTTP_REPLY2[] PROGMEM = "HTTP/1.1 200 OK\r\nConnection: Closed\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 71\r\n\r\n<html><body><h1>Success!</h1><p>This is page /test.</p></body></html>\r\n";
/**
 * HTTP/1.1 200 OK\r\n
 * Connection: Closed\r\n
 * Content-Type: text/html; charset=utf-8\r\n
 * Content-Length: 71\r\n
 * \r\n
 * <html><body><h1>Success!</h1><p>This is page /test.</p></body></html>\r\n
 */

ESP8266_HTTP server(RX_PIN, TX_PIN, RST_PIN);
WifiMessage *msg = NULL;
Route *route = NULL;
byte code = 0;


void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("ESP8266_HTTP_SERVER test sketch!");

    if (server.start(SSID, PASS, PORT) == 0) {
        Serial.print("Server is running on ");
        Serial.print(server.getIP());
        Serial.print(":");
        Serial.println(PORT);

        // Every Route has unique ID - it is given by the sequence of registration
        server.registerRoute(HTTP_Method::GET, "/"); // ID == 1
        server.registerRoute(HTTP_Method::GET, "/test"); // ID == 2
    }
}


void loop() {
    code = server.update();
    switch(code) {
        case 4: // Something was received
            Serial.println("Unexpected Message from ESP8266!");
            break;
        case 3: // TCP Message
            Serial.println("New Message!");
            route = server.preprocessRequest();
            processRequest(route);
            break;
        case 2: // Client disconnected
            Serial.println("Disconnected!");
            break;
        case 1: // Client connected
            Serial.println("Connected!");
            break;
        case 0: // Nothing happened
        default:
            break;
    }

    // Rest of Arduino code
    // Must be fast code
}

void processRequest(Route * route) {
    if (route != NULL) {
        // Message is HTTP request and the request is registered
        msg = server.getWifiMessage();
        switch(route->getID()) {
            case 2:
                Serial.println("GET /test HTTP Request.");
                if (route->getParams() != NULL) {
                    // Has params
                    Serial.print("params: ");
                    Serial.println(route->getParams());
                    // Here dissect params ...
                    // tokenize first line of the message
                    //char * pkey = strtok(route->getParams(), "=&");
                    //char * pvalue = strtok(NULL, "=&");
                }
                // Here update Arduino state ...

                // Send response
                server.send_PROGMEM(msg->channel, PROGMEM_HTTP_REPLY2); // Custom static HTTP response
                server.closeConnection(msg->channel);
                break;
            case 1:
                Serial.println("GET / HTTP Request.");
                // Here update Arduino state ...

                // Send response
                server.send200();
                server.closeConnection(msg->channel);
                break;
            case 0:
            default:
                break;
        }
    }
}
