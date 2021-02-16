/*
 * Created by Jakub Svajka on 2021-02-11.
 * 
 * Based on SerialESP8266wifi.h by Jonas Ekstrand on 2015-02-20.
 * ESP8266 AT commands reference: https://github.com/espressif/esp8266_at/wiki
 */
#include "ESP8266_WLAN.h"

#define RX_PIN  4  // Connect this pin to TX on the esp8266
#define TX_PIN  6  // Connect this pin to RX on the esp8266
#define RST_PIN 5

#define SSID "ssid1234"
#define PASS "pass1234"
#define PORT "2121"


ESP8266_WLAN wifi(RX_PIN, TX_PIN, RST_PIN);
WifiMessage *msg;

byte code = 0;

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("ESP8266_WLAN_TCP_CLI test sketch!");

    if (wifi.init()) {
        Serial.println("Successful initialization of ESP8266!");
        if(wifi.connectToAP(SSID, PASS)) {
            Serial.println("Successfully connected to Access Point!");
            Serial.print("IP: ");
            Serial.println(wifi.getIP());
            Serial.print("MAC: ");
            Serial.println(wifi.getMAC());

            if(wifi.createTCPServer(PORT)) {
                Serial.print("Successfully created TCP server at port ");
                Serial.print(PORT);
                Serial.println("!");

                Serial.print("STATUS:");
                Serial.println(wifi.getStatus());
            }
            else {
                Serial.println("[3]: ESP8266 isn't working...");
            }
        }
        else {
            Serial.println("[2]: ESP8266 isn't working...");
        }
    }
    else {
        Serial.println("[1]: ESP8266 isn't working...");
    }
}

void loop() {
    // Checks serial
    code = wifi.update();
    switch(code) {
        case 4: // Something was received
            Serial.println("Unexpected Message from ESP8266!");
            break;
        case 3: // TCP Message
            msg = wifi.getWifiMessage();
            Serial.print("message:\"");
            Serial.print(msg->message);
            Serial.println("\"");

            wifi.send(msg->channel, "Hello, World!");
            if (!wifi.closeConnection(msg->channel))
              Serial.println("Unsuccessful!");
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
}
