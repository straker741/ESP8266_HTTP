#include "ESP8266_WLAN.h"

// Responses
const char PROGMEM_OK[] PROGMEM = "OK";
const char PROGMEM_FAIL[] PROGMEM = "FAIL";
const char PROGMEM_ERROR[] PROGMEM = "ERROR";
const char PROGMEM_SEND_OK[] PROGMEM = "SEND OK";
const char PROGMEM_READY[] PROGMEM = "ready";
const char PROGMEM_CONNECT[] PROGMEM = "CONNECT";
const char PROGMEM_CLOSED[] PROGMEM = "CLOSED";
const char PROGMEM_STAIP[] PROGMEM = "STAIP,\"";
const char PROGMEM_STAMAC[] PROGMEM = "STAMAC,\"";
const char PROGMEM_STATUS[] PROGMEM = "STATUS:";
const char PROGMEM_WIFI_DISCONNECT[] PROGMEM = "WIFI DISCONNECT";

// Requests
const char PROGMEM_CIPSTATUS[] PROGMEM = "AT+CIPSTATUS";
const char PROGMEM_CWMODE_1[] PROGMEM = "AT+CWMODE=1";
const char PROGMEM_CIPMUX_1[] PROGMEM = "AT+CIPMUX=1";
const char PROGMEM_AT[] PROGMEM = "AT";
const char PROGMEM_ATE1[] PROGMEM = "ATE1";
const char PROGMEM_RST[] PROGMEM = "AT+RST";
const char PROGMEM_GMR[] PROGMEM = "AT+GMR";
const char PROGMEM_CWJAP[] PROGMEM = "AT+CWJAP=\"";
const char PROGMEM_CWQAP[] PROGMEM = "AT+CWQAP";
const char PROGMEM_CIFSR[] PROGMEM = "AT+CIFSR";
const char PROGMEM_CIPSERVER_START[] PROGMEM = "AT+CIPSERVER=1,";
const char PROGMEM_CIPSERVER_STOP[] PROGMEM = "AT+CIPSERVER=0";
const char PROGMEM_CIPCLOSE[] PROGMEM = "AT+CIPCLOSE=";
const char PROGMEM_CIPSEND[] PROGMEM = "AT+CIPSEND=";
const char PROGMEM_IPD[] PROGMEM = "+IPD,";


// Constructor
ESP8266_WLAN::ESP8266_WLAN(byte RX_PIN, byte TX_PIN, byte RST_PIN):
SoftwareSerial::SoftwareSerial(RX_PIN, TX_PIN)
{
    SoftwareSerial::begin(9600);

    _RST_PIN = RST_PIN;
    pinMode(_RST_PIN, OUTPUT);
    digitalWrite(_RST_PIN, LOW); // Reset ESP8266

    _flags.initialized = false;
    _flags.connectedToAP = false;
    _flags.tcpServerRunning = false;
    _flags.sending = false;
    _flags.unexpectedEcho = false;

    for (byte i = 0; i < MAX_CONNECTIONS; i++) {
        _connections[i].channel = i + '0';
        _connections[i].connected = false;
    }
    memset(BUFFER, '\0', MAX_BUFFER_SIZE);
    CUR_BUFFER_SIZE = 0;
}


// Destructor
ESP8266_WLAN::~ESP8266_WLAN() {}


/**
 * @brief Executes "AT" AT command.
 * @return true when ESP8266 responds with "OK".
 */
bool ESP8266_WLAN::isActive() {
    writeCommand(PROGMEM_AT);
    return (checkResponse() == 1);
}


/**
 * @brief Executes initialization of ESP8266
 * @return true when successfully initialized.
 */
bool ESP8266_WLAN::init() {
    _flags.initialized = false;
    _flags.connectedToAP = false;
    _flags.tcpServerRunning = false;
    _flags.sending = false;
    _flags.unexpectedEcho = false;

    // Do a HW restart
    if (!hardRestart()) {
        return false;
    }

    // Turn on echo
    writeCommand(PROGMEM_ATE1);
    if (checkResponse() != 1)
        return false;

    // Set Station mode (No SoftAP)
    writeCommand(PROGMEM_CWMODE_1);
    if (checkResponse() != 1)
        return false;

    // Allow multiple connections
    writeCommand(PROGMEM_CIPMUX_1);
    if (checkResponse() != 1)
        return false;

    _flags.initialized = true;
    return true;
}


/**
 * @brief Tries to connect to Access Point.
 * @param ssid Access Point Identifier
 * @param pass Passphrase
 * @return true when successful.
 */
bool ESP8266_WLAN::connectToAP(const char * ssid, const char * pass) {
    strncpy(_ssid, ssid, sizeof(_ssid));
    strncpy(_pass, pass, sizeof(_pass));
    return connectToAP();
}


/**
 * @brief Tries to connect to Access Point with credentials saved in _ssid and _pass.
 * @return true when successful.
 */
bool ESP8266_WLAN::connectToAP() {
    writeCommand(PROGMEM_CWJAP, false);
    print(_ssid);
    print("\",\"");
    print(_pass);
    println("\"");

    if (checkResponse() != 1)
        return false;
    _flags.connectedToAP = true;
    return true;
}


/**
 * @brief Disconnect from Access Point.
 * @return true when successfully disconnected from Access Point.
 */
bool ESP8266_WLAN::disconnectFromAP() {
    writeCommand(PROGMEM_CWQAP);
    if (checkResponse() != 1)
        return false;
    _flags.connectedToAP = false;
    return true;
}


/**
 * @brief Performs an AT command to request static IPv4 address.
 * @return IPv4 address of ESP8266.
 */
char * ESP8266_WLAN::getIP() {
    writeCommand(PROGMEM_CIFSR);
    if (!readCommand(PROGMEM_CIFSR))
        return "";
    if (readData() != 1)
        return "";

    // Data are stored in BUFFER
    const char * ps = strstr_P(BUFFER, PROGMEM_STAIP) + strlen_P(PROGMEM_STAIP);
    const char * pe = strchr(ps, '"');
    byte len = pe - ps;
    memcpy(_ip, ps, len);
    _ip[len] = '\0';
    return _ip;
}


/**
 * @brief Performs an AT command to request MAC address.
 * @return MAC address of ESP8266 in string format: xx:xx:xx:xx:xx:xx
 */
char* ESP8266_WLAN::getMAC() {
    writeCommand(PROGMEM_CIFSR);
    if (!readCommand(PROGMEM_CIFSR))
        return "";
    if (readData() != 1)
        return "";

    // Data are stored in BUFFER
    const char * ps = strstr_P(BUFFER, PROGMEM_STAMAC) + strlen_P(PROGMEM_STAMAC);
    const char * pe = strchr(ps, '"');
    byte len = pe - ps;
    memcpy(_mac, ps, len);
    _mac[len] = '\0';
    return _mac;
}


/**
 * @brief Checks whether the ESP8266 is connected to Access Point.
 */
bool ESP8266_WLAN::isConnectedToAP() {
    return (getStatus() != '5');
}


/**
 * @return true when successfully created server
 */
bool ESP8266_WLAN::createTCPServer(const char * port) {
    strncpy(_port, port, sizeof(_port));
    return createTCPServer();
}


bool ESP8266_WLAN::createTCPServer() {
    writeCommand(PROGMEM_CIPSERVER_START, false);
    println(_port);
    if (checkResponse() != 1)
        return false;
    _flags.tcpServerRunning = true;
    return true;
}


/**
 * @return true when successfully deleted server
 */
bool ESP8266_WLAN::deleteTCPServer() {
    writeCommand(PROGMEM_CIPSERVER_STOP);
    if (checkResponse() == 1) {
        _flags.tcpServerRunning = false;
        return true;
    }
    else {
        _flags.tcpServerRunning = true;
        return false;
    }
}


/**
 * 0 : Unexpected echo - echo is stored in BUFFER
 * 1 : Error
 * 2 : Got IP (Connected to Access Point)
 * 3 : Connected (At least one client is connected)
 * 4 : Disconnected (No one is connected)
 * 5 : No IP (Not connected to Access Point)
 */
char ESP8266_WLAN::getStatus() {
    writeCommand(PROGMEM_CIPSTATUS);
    if (!readCommand(PROGMEM_CIPSTATUS)) {
        _flags.unexpectedEcho = true;
        return '0';
    }
    if (readData() != 1)
        return '1';

    // Data are stored in BUFFER
    const char * p = strstr_P(BUFFER, PROGMEM_STATUS) + strlen_P(PROGMEM_STATUS);
    return (*p);
}


bool ESP8266_WLAN::closeConnection(char channel) {
    writeCommand(PROGMEM_CIPCLOSE, false);
    println(channel);

    // AT+CIPCLOSE=0
    // 0,CLOSED
    // OK
    CUR_BUFFER_SIZE = readLine(BUFFER, MAX_BUFFER_SIZE);
    if (strstr_P(BUFFER, PROGMEM_CIPCLOSE) == NULL) {
        _flags.unexpectedEcho = true;
        return false;
    }
    if (readData() != 1)
        return false;
    // TODO?
    // sanitize BUFFER - temporary solution - better would be tell readData to not append \r\n
    char * p = strstr_P(BUFFER, PROGMEM_CLOSED) + strlen_P(PROGMEM_CLOSED);
    p[0] = '\0';
    _flags.unexpectedEcho = true;
    return true;
}


/**
 * @brief Checks to see if there is any client connected.
 * @return false when there is no established connection.
 */
bool ESP8266_WLAN::anyClientConnected() {
    for (unsigned int i = 0; i < MAX_CONNECTIONS; i++) {
        if (_connections[i].connected) {
            return true;
        }
    }
    return false;
}


/**
 * @brief Sends message. Function assumes the channel is connected!
 * @param channel Channel to which the message should be sent.
 * @param message End every line with
 * @param eol End Of Line flag
 * @param sendNow false - message is saved only to the buffer.
 * @return true when success.
 */
bool ESP8266_WLAN::send(char channel, String& message, bool eol, bool sendNow) {
    return send(channel, message.c_str(), eol, sendNow);
}


bool ESP8266_WLAN::send(char channel, const char * message, bool eol, bool sendNow) {
    // Set flag "sending" if first send command
    if (!_flags.sending) {
        _flags.sending = true;
        CUR_BUFFER_SIZE = 0;
    }

    // Copy contents of the message to the BUFFER
    strcpy(&BUFFER[CUR_BUFFER_SIZE], message);
    // Update CUR_BUFFER_SIZE
    CUR_BUFFER_SIZE += strlen(message);

    if (eol) {
        BUFFER[CUR_BUFFER_SIZE++] = '\r';
        BUFFER[CUR_BUFFER_SIZE++] = '\n';
        BUFFER[CUR_BUFFER_SIZE] = '\0';
    }

    if (!sendNow)
        return true;

    writeCommand(PROGMEM_CIPSEND, false);
    print(channel);
    print(",");
    println(CUR_BUFFER_SIZE);

    if (checkResponse() == 1) {
        print(BUFFER); // send message
        if (checkResponse() == 4) {
            _flags.sending = false;
            return true;
        }
    }
    _flags.sending = false;
    return false;
}

/**
 * message is loaded from Flash (PROGMEM). MUST END WITH \r\n!
 */
bool ESP8266_WLAN::send_PROGMEM(char channel, const char * message) {
    _flags.sending = true;
    strcpy_P(BUFFER, message);
    CUR_BUFFER_SIZE = strlen_P(message);

    writeCommand(PROGMEM_CIPSEND, false);
    print(channel);
    print(",");
    println(CUR_BUFFER_SIZE);

    if (checkResponse() == 1) {
        print(BUFFER); // send message
        if (checkResponse() == 4) {
            _flags.sending = false;
            return true;
        }
    }
    _flags.sending = false;
    return false;
}


/**
 * 0 : Nothing happened
 * 1 : Client connected
 * 2 : Client disconnected
 * 3 : TCP message
 * 4 : Response to AT request (e. g. AT+CIPSTATUS, etc.)
 */
byte ESP8266_WLAN::update() {
    // Resolve wifi message first
    if (msg.hasData) {
        msg.overflowed = false;
        msg.hasData = false;
        msg.channel = '\0';
        msg.message = NULL;
    }
    if (SoftwareSerial::available() || _flags.unexpectedEcho) {
        // Get first line if no unexpectedEcho
        if (!_flags.unexpectedEcho)
            CUR_BUFFER_SIZE = readLine(BUFFER, MAX_BUFFER_SIZE);

        if (CUR_BUFFER_SIZE == 0) {
            // \r\n - definitelly not interesting => Treat as nothing happened
            return 0;
        }
        if (strcmp_P(BUFFER + 2, PROGMEM_CONNECT) == 0) {
            // Client connected
            // TODO: Close connection if channel is higher than MAX_CONNECTIONS
            byte channel = BUFFER[0] - '0';
            _connections[channel].connected = true;
            _flags.unexpectedEcho = false;
            return 1;
        }
        if (strcmp_P(BUFFER + 2, PROGMEM_CLOSED) == 0) {
            // Client disconnected
            byte channel = BUFFER[0] - '0';
            _connections[channel].connected = false;
            _flags.unexpectedEcho = false;
            return 2;
        }
        if (strstr_P(BUFFER, PROGMEM_IPD) != NULL) {
            // TCP message
            updateWifiMessage();
            return 3;
        }

        // ESP8266 may malfunctions
        // TODO: Implement some kind of mechanism to recognize malfunction
        /*if (diagnose()) {
            _flags.unexpectedEcho = false;
            return 4;
        }*/
        _flags.unexpectedEcho = false;
    }
    return 0;
}


/**
 * @brief Gets incoming TCP message. Note that this function is blocking and buffer may overflow
 * @return 0 - no message, 1 - new message
 */
void ESP8266_WLAN::updateWifiMessage() {
    // First line is already in BUFFER
    // Getting channel
    char channel = BUFFER[5];

    // Getting size of message: msg_size
    const char * startOfMessageSize = &BUFFER[7];
    const char * startOfMessage = strchr(startOfMessageSize, ':') + 1;
    size_t len = startOfMessage - startOfMessageSize;
    char buf[len];
    memcpy(buf, startOfMessageSize, len - 1);
    buf[len - 1] = '\0';
    size_t msg_size = atoi(buf);

    // Whole message might not be in the BUFFER
    size_t firstLineSize = strlen(startOfMessage) + 2; // +2 whitespaces \r\n

// For now ignore overflow - assume that overflow is not possible
    /*if (CUR_BUFFER_SIZE >= (MAX_BUFFER_SIZE - 1)) {
        // Overflow
        msg.overflowed = true;
    }*/
    //else if (firstLineSize == msg_size) {
    if (firstLineSize == msg_size) {
        // Whole message is already in the BUFFER 
        // function readLine stops reading when \r (does not include \r into the message)
        //msg.overflowed = false;
        ;
    }
    else {
        // Only part of the message is in the BUFFER
        // function readLine stops reading when \r (does not include \r into the message)
        BUFFER[CUR_BUFFER_SIZE++] = '\r';
        BUFFER[CUR_BUFFER_SIZE++] = '\n';
        // Read the rest of the message
        CUR_BUFFER_SIZE += readBytes(&BUFFER[CUR_BUFFER_SIZE], msg_size - firstLineSize);
        //msg.overflowed = false;
    }
    msg.hasData = true;
    msg.channel = channel;
    msg.message = startOfMessage;

    _flags.unexpectedEcho = false;
    return;
}

// Returns pointer to msg
WifiMessage * ESP8266_WLAN::getWifiMessage() {
    return &msg;
}


/**
 * Looks for any messages which would idicate ESP8266 malfunctions. 
 * Workst case Arduino resets.
 * @return true - No issues; false - Issues recognized but cleared
 */
bool ESP8266_WLAN::diagnose() {
    if (strcpy_P(BUFFER, PROGMEM_WIFI_DISCONNECT) == 0) {
        if (!restart()) {
            // restart() failed => Reset Arduino
            asm("  jmp 0");
        }
        return false;
    }
    // Terrible function - it gets stuck in checkResponse() setTimeout is valid only for Stream::ReadBytes()
    setTimeout(10000); // 10 seconds
    if (checkResponse() == 5) {
        // "ready" message detected
        if (!restart()) {
            // restart() failed => Reset Arduino
            asm("  jmp 0");
        }
        return false;
    }
    setTimeout(1000);
    return true;
}


/**
 * @brief Executes init(), connectToAP() and createTCPServer() in the right order.
 * @return true when successful.
 */
bool ESP8266_WLAN::restart() {
    return init() && connectToAP() && createTCPServer();
}


/**
 * @brief Executes software restart issuing "AT+RST" AT command.
 * @return true when successfully restarted and ready for operation.
 */
bool ESP8266_WLAN::softRestart() {
    writeCommand(PROGMEM_RST);
    return (checkResponse() == 5);
}


/**
 * @brief Executes hardware restart by pulling _RST_PIN to LOW.
 * @return true when successfully restarted and ready for operation.
 */
bool ESP8266_WLAN::hardRestart() {
    digitalWrite(_RST_PIN, LOW);
    delay(500);
    digitalWrite(_RST_PIN, HIGH);
    return (checkResponse() == 5);
}


byte ESP8266_WLAN::readData() {
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
byte ESP8266_WLAN::readData(size_t origin) {
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
byte ESP8266_WLAN::checkResponse() {
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
void ESP8266_WLAN::writeCommand(const char * cmd, bool eol) {
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
bool ESP8266_WLAN::readCommand(const char * cmd, bool progmem) {
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
size_t ESP8266_WLAN::readLine(char * buf, size_t len) {
    // One line may end by <\r><\r><\n> or <\r><\n>
    size_t b = readBytesUntil('\r', buf, len - 1);
    buf[b] = '\0';
    while (read() != '\n'); // Discard rest of the line
    return b;
}
