#include "ESP8266_AT.h"


const char PROGMEM_OK[] PROGMEM = "OK";
const char PROGMEM_FAIL[] PROGMEM = "FAIL";
const char PROGMEM_ERROR[] PROGMEM = "ERROR";
const char PROGMEM_SEND_OK[] PROGMEM = "SEND OK";
const char PROGMEM_READY[] PROGMEM = "ready";


// Constructor
ESP8266_AT::ESP8266_AT(byte RX_PIN, byte TX_PIN):
SoftwareSerial::SoftwareSerial(RX_PIN, TX_PIN)
{
    memset(BUFFER, '\0', MAX_BUFFER_SIZE);
    CUR_BUFFER_SIZE = 0;
    SoftwareSerial::begin(9600);
}


// Destructor
ESP8266_AT::~ESP8266_AT() {}


byte ESP8266_AT::readData() {
    return readData(0);
}

/**
 * Reads Data to BUFFER until finds valid AT response.
 * 0 : Buffer overflowed
 * 1 : "OK"
 * 2 : "FAIL"
 * 3 : "ERROR"
 * 4 : "SEND OK"
 * 5 : "ready"
 */
byte ESP8266_AT::readData(size_t origin) {
    CUR_BUFFER_SIZE = origin;
    byte code = 0;
    // First line - Save Data to the start of the BUFFER
    byte DATA_SIZE = readLine(BUFFER + CUR_BUFFER_SIZE, MAX_BUFFER_SIZE);

    while (1) {
        if ((CUR_BUFFER_SIZE + DATA_SIZE + 2) > MAX_BUFFER_SIZE)
            return 0;
        if (strcmp_P(BUFFER + CUR_BUFFER_SIZE, PROGMEM_OK) == 0) {
            code = 1;
            break;
        }
        if (strcmp_P(BUFFER + CUR_BUFFER_SIZE, PROGMEM_FAIL) == 0) {
            code = 2;
            break;
        }
        if (strcmp_P(BUFFER + CUR_BUFFER_SIZE, PROGMEM_ERROR) == 0) {
            code = 3;
            break;
        }
        if (strcmp_P(BUFFER + CUR_BUFFER_SIZE, PROGMEM_SEND_OK) == 0) {
            code = 4;
            break;
        }
        if (strcmp_P(BUFFER + CUR_BUFFER_SIZE, PROGMEM_READY) == 0) {
            code = 5;
            break;
        }
        // Message is not finished - add new line
        CUR_BUFFER_SIZE += DATA_SIZE;
        BUFFER[CUR_BUFFER_SIZE++] = '\r';
        BUFFER[CUR_BUFFER_SIZE++] = '\n';
        // Read next line - Note that buffer may overflow - Stops reading mid line when no more space
        DATA_SIZE = readLine(BUFFER + CUR_BUFFER_SIZE, MAX_BUFFER_SIZE - CUR_BUFFER_SIZE);
    }
    BUFFER[CUR_BUFFER_SIZE - DATA_SIZE + 2] = '\0';
    return code;
}


/**
 * Reads until finds valid AT response. Discards data of the message.
 * 1 : "OK"
 * 2 : "FAIL"
 * 3 : "ERROR"
 * 4 : "SEND OK"
 * 5 : "ready"
 */
byte ESP8266_AT::checkResponse() {
    char buf[20];
    size_t len = 20;
    while (1) {
        readLine(buf, len);

        if (strcmp_P(buf, PROGMEM_OK) == 0)
            return 1;
        if (strcmp_P(buf, PROGMEM_FAIL) == 0)
            return 2;
        if (strcmp_P(buf, PROGMEM_ERROR) == 0)
            return 3;
        if (strcmp_P(buf, PROGMEM_SEND_OK) == 0)
            return 4;
        if (strcmp_P(buf, PROGMEM_READY) == 0)
            return 5;
    }
    return 0;
}


/**
 * @brief Writes commands (from PROGMEM) to serial output
 * @param cmd const PROGMEM char *: command
 * @param eol End Of Line flag: print CRLF as well?
 */
void ESP8266_AT::writeCommand(const char * cmd, bool eol) {
    char buf[20];
    strcpy_P(buf, (char *)cmd);
    if (eol)
        println(buf);
    else
        print(buf);
}


/**
 * Reads from serial input one line and compares with cmd param. 
 * Use this function when awaiting response to a certain command issued right before. 
 * @param cmd expected command
 * @param progmem leave true when passing directly from PROGMEM otherwise false
 * @return true when expected command found otherwise false
 */
bool ESP8266_AT::readCommand(const char * cmd, bool progmem) {
    do {
        CUR_BUFFER_SIZE = readLine(BUFFER, MAX_BUFFER_SIZE);
    } while (CUR_BUFFER_SIZE == 0);
    if (progmem) {
        return (strcmp_P(BUFFER, cmd) == 0);
    }
    else {
        return (strcmp(BUFFER, cmd) == 0);
    }
}


/**
 * @brief Reads from serial stream to buf until Carriage Return found.
 * @param buf Buffer to which the data will be put.
 * @param len Maximum size of the buffer.
 * @return Number of bytes saved in the buffer.
 */
size_t ESP8266_AT::readLine(char * buf, size_t len) {
    // One line may end by <\r><\r><\n> or <\r><\n>
    size_t b = readBytesUntil('\r', buf, len - 1);
    buf[b] = '\0';
    while (read() != '\n'); // Discard rest of the line
    return b;
}
