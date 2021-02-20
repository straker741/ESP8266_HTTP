/*
 * Created by Jakub Svajka on 2021-02-19.
 */
#include "ESP8266_HTTP.h"

#define RX_PIN  4  // Connect this pin to TX on the esp8266
#define TX_PIN  6  // Connect this pin to RX on the esp8266
#define RST_PIN 5

#define SSID "ssid1234"
#define PASS "pass1234"
#define PORT "80"

void processRequest(Route * route);

ESP8266_HTTP server(RX_PIN, TX_PIN, RST_PIN);
WifiMessage *msg = NULL;
Route *route = NULL;
byte code = 0;


void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("Initialization...");

    if (server.start(SSID, PASS, PORT) == 0) {
        Serial.print("Server is running on ");
        Serial.print(server.getIP());
        Serial.print(":");
        Serial.println(PORT);

        // Every Route has unique ID - it is given by the sequence of registration
        server.registerRoute(HTTP_Method::GET, "/on"); // ID == 1
        server.registerRoute(HTTP_Method::GET, "/off"); // ID == 1
    }

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
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
                Serial.println("GET /off HTTP Request.");
                // Here update Arduino state ...
                digitalWrite(LED_BUILTIN, LOW);

                // Send response
                server.send200();
                server.send(msg->channel);
                server.closeConnection(msg->channel);
                break;
            case 1:
                Serial.println("GET /on HTTP Request.");
                // Here update Arduino state ...
                digitalWrite(LED_BUILTIN, HIGH);

                // Send response
                server.send200();
                server.send(msg->channel);
                server.closeConnection(msg->channel);
                break;
            case 0:
            default:
                break;
        }
    }
}