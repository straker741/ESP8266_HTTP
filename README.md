# ESP8266_HTTP
ESP8266_HTTP is a simple and lightweight library designed to allow the user to easily interconnect Arduino with generic ESP8266 and manage simple HTTP server. Communication with ESP8266 is accomplished via serial by using AT instruction set. The library is built on SoftwareSerial library, leaving hardware serial for debugging.

## Basic usage
ESP8266_HTTP library is mainly composed of two high level classes: ESP8266_HTTP and ESP8266_WLAN. Each of them has a clear and specific function in the whole application. Class ESP8266_HTTP enables user to manage simple HTTP requests while class ESP8266_WLAN is responsible for managing communication between ESP8266 and Arduino by using AT instruction set which includes receiving, decoding, processing and storing message into the BUFFER.

In case of incomming HTTP request that is not registered, method preprocessRequest() automatically responds with 404 NOT FOUND and returns NULL pointer.

## Example
```cpp
#include "ESP8266_HTTP.h"

#define RX_PIN  4  // Connect this pin to TX on the esp8266
#define TX_PIN  6  // Connect this pin to RX on the esp8266
#define RST_PIN 5

#define SSID "ssid1234"
#define PASS "pass1234"
#define PORT "80"

ESP8266_HTTP server(RX_PIN, TX_PIN, RST_PIN);

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("Initializating...");

    if (server.start(SSID, PASS, PORT) == 0) {
        Serial.print("Server is running on ");
        Serial.print(server.getIP());
        Serial.print(":");
        Serial.println(PORT);

        // Every Route has unique ID - it is given by the sequence of registration
        server.registerRoute(HTTP_Method::GET, "/");     // ID == 1
        server.registerRoute(HTTP_Method::GET, "/test"); // ID == 2
    }
}

void loop() {
    code = server.update();
    switch(code) {
        case 4: // Something was received
            Serial.println("AT message from ESP8266!");
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
    // Must be fast (non-blocking) code
}

void processRequest(Route * route) {
    if (route != NULL) {
        // Valid HTTP request
        // Here process request
        // See other examples to learn how the processRequest function might look like
    }
}
```

## Notable methods
### udpate()
Checks serial for any incomming messages. If there is a message it automatically parses the message and updates the state. Make sure method update() is called every loop cycle.
```cpp
/**
 * 0 : Nothing happened
 * 1 : Client connected
 * 2 : Client disconnected
 * 3 : TCP message
 * 4 : Response to AT request (e. g. AT+CIPSTATUS, etc.)
 */
byte ESP8266_WLAN::update();
```

### send()
Use this method when responding to a TCP message (return code 3 of update()). This method presumes that the "message" argument is string literal saved in RAM. If the "message" argument is saved in FLASH (PROGMEM) then use method send_PROGMEM(). Note that It is possible to just use print() and println() methods of Print class.
```cpp
/**
 * @brief Sends message. Function assumes the channel is connected!
 * @param channel Channel to which the message should be sent.
 * @param message End every line with
 * @param eol End Of Line flag
 * @param sendNow false - message is saved only to the BUFFER.
 * @return true when success.
 */
bool ESP8266_WLAN::send(char channel, const char * message, bool eol, bool sendNow);
```

### send_PROGMEM()
Use this method when responding to a TCP message. This method presumes that the "message" argument is string literal saved in FLASH (PROGMEM)
```cpp
/**
 * @brief Message is loaded from Flash (PROGMEM). MUST END WITH \r\n !!!
 * @param channel Channel to which the message should be sent.
 * @param message End every line with
 */
bool ESP8266_WLAN::send_PROGMEM(char channel, const char * message);
```

### preprocessRequest()
Takes care of every request which is not registered.
```cpp
/**
 * Checks whether it is HTTP request. Then it takes apart the request. 
 * If the request is not registered, then the response will be NOT FOUND (404), 
 * @return Pointer to Route object which was requested, otherwise NULL
 */
Route * ESP8266_HTTP::preprocessRequest();
```

## Constants
Make sure the following constants suit your application.

| Constant           | Default Value | Description |
|:------------------ |:-------------:|:----------- |
| MAX_ROUTES         | 3             | Defines how many routes are possible to register. Bigger application would certainly require more than 3. |
| MAX_BUFFER_SIZE*   | 512           | Defines the size of the BUFFER where all messages are saved. Typical HTTP request has around 350** bytes. |
| MAX_CONNECTIONS    | 3             | Defines how many clients can be connected at the same time. (Number of independent channels.) |
| MAX_RESET_ATTEMPTS | 3             | For now not used. |

\* Arduino Nano and Uno have only 2048 bytes of RAM. It is possible to increase MAX_BUFFER_SIZE but make sure the Global variable size is around 70%-80% at max.

\*\* When dealing only with GET methods, then most of the time only the first line of the request is needed.

## Schematic
<img src="img/schematic.png" alt="Wiring schematic of ESP8266">

<img src="img/breadboard.jpg" alt="My wiring of ESP8266 on breadboard">


## Known issues and limitations
* size of BUFFER: Able to hold only up to 500 bytes.
* no collision detection
* SoftwareSerial's serial speed is limited (default 9600 baud)
* It is forbidden to issue AT requests in every loop cycle - ESP8266 is not able to respond that fast. Plus you might miss a message from ESP8266 regarding cases 1, 2 and 3 of update() method.

## ESP8266 AT commands reference:
* https://github.com/espressif/esp8266_at/wiki
* https://www.espressif.com/sites/default/files/documentation/4a-esp8266_at_instruction_set_en.pdf
* https://www.itead.cc/wiki/images/5/53/Esp8266_at_instruction_set_en_v1.5.4_0.pdf