#include "ESP8266_HTTP.h"


const char PROGMEM_HTTP_OK[] PROGMEM = "HTTP/1.1 200 OK\r\nConnection: Closed\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 45\r\n\r\n<html><body><h1>Success!</h1></body></html>\r\n";
/**
 * HTTP/1.1 200 OK\r\n
 * Connection: Closed\r\n
 * Content-Type: text/html; charset=utf-8\r\n
 * Content-Length: 45\r\n
 * \r\n
 * <html><body><h1>Success!</h1></body></html>\r\n
 */

const char PROGMEM_HTTP_NOT_FOUND[] PROGMEM = "HTTP/1.1 404 NOT FOUND\r\nConnection: Closed\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 67\r\n\r\n<html><body><h1>Requested page does not exist!</h1></body></html>\r\n";
/**
 * HTTP/1.1 404 NOT FOUND\r\n
 * Connection: Closed\r\n
 * Content-Type: text/html; charset=utf-8\r\n
 * Content-Length: 67\r\n
 * \r\n
 * <html><body><h1>Requested page does not exist!</h1></body></html>\r\n
 */


/*******************************
 * ---------- ROUTE ---------- *
 *******************************/
// Constructor
Route::Route() {
    _id = 0;
    _method = HTTP_Method::HTTP_METHOD_LENGTH;
    _path = NULL;
    _params = NULL;
}


// Destructor
Route::~Route() {}


/**
 * @param method HTTP_Method enum specifies HTTP method.
 * @param path corresponding URL path with the method (example: "/test").
 */
Route::Route(HTTP_Method method, const char * path) {
    set(0, method, path);
}


void Route::set(byte ID, HTTP_Method method, const char * path) {
    _id = ID;
    _method = method;
    size_t len = strlen(path);
    _path = new char[len + 1];
    strcpy(_path, path);
}


/**
 * @brief Sets the received parameters of the evoked route
 * @param params string of params in format for example a=5&b=7, NULL does not save anything
 */
void Route::setParams(const char * params) {
    if (params == NULL) {
        _params = NULL;
    }
    else {
        size_t len = strlen(params);
        _params = new char[len + 1];
        strcpy(_params, params);
    }
}


bool Route::operator==(const Route & route) {
    return (_method == route._method) && (strcmp(_path, route._path) == 0);
}


/********************************
 * ---------- ROUTER ---------- *
 ********************************/
// Constructor
Router::Router() {
    _size = 0;
}


// Destructor
Router::~Router() {}


/**
 * @brief Routes which are not registered will be automatically refused with 404 NOT FOUND.
 * @param method HTTP_Method enum specifies HTTP method.
 * @param path corresponding URL path with the method (example: "/test").
 */
void Router::registerRoute(HTTP_Method method, const char * path) {
    _routes[_size].set(_size + 1, method, path);
    _size++;
}


/**
 * @brief Chceks whether the requested route is registered.
 * @return *Route - pointer to the route which was requested, otherwise NULL.
 */
Route * Router::isRegistered(const char * method, const char * path) {
    if (strcmp("GET", method) == 0)
        return isRegistered(HTTP_Method::GET, path);
    if (strcmp("HEAD", method) == 0)
        return isRegistered(HTTP_Method::HEAD, path);
    if (strcmp("POST", method) == 0)
        return isRegistered(HTTP_Method::POST, path);
    if (strcmp("PUT", method) == 0)
        return isRegistered(HTTP_Method::PUT, path);
    if (strcmp("DELETE", method) == 0)
        return isRegistered(HTTP_Method::DELETE, path);
    if (strcmp("TRACE", method) == 0)
        return isRegistered(HTTP_Method::TRACE, path);
    if (strcmp("OPTIONS", method) == 0)
        return isRegistered(HTTP_Method::OPTIONS, path);
    if (strcmp("CONNECT", method) == 0)
        return isRegistered(HTTP_Method::CONNECT, path);
    if (strcmp("PATCH", method) == 0)
        return isRegistered(HTTP_Method::PATCH, path);
    return NULL;
}


/**
 * @brief Chceks whether the requested route is registered.
 * @return *Route - pointer to the route which was requested, otherwise NULL.
 */
Route * Router::isRegistered(HTTP_Method method, const char * path) {
    Route route = Route(method, path);
    for (size_t index = 0; index < _size; index++) {
        if (_routes[index] == route)
            return &_routes[index];
    }
    return NULL;
}


/**************************************
 * ---------- ESP8266_HTTP ---------- *
 **************************************/
// Constructor
ESP8266_HTTP::ESP8266_HTTP(byte RX_PIN, byte TX_PIN, byte RST_PIN):
ESP8266_WLAN::ESP8266_WLAN(RX_PIN, TX_PIN, RST_PIN),
Router::Router()
{

}


// Destructor
ESP8266_HTTP::~ESP8266_HTTP() {}


/**
 * @brief Initializes ESP8266, connects to Access Point and creates TCP server.
 * @param ssid Access Point Identifier.
 * @param pass Passphrase.
 * @param port Port at which to create TCP server.
 * @return 0 - Success; 1 - Error: Unable to initialize ESP8266; 2 - Error: Unable to connect to Access Point; 3 - Error: Unable to start TCP server
 */
byte ESP8266_HTTP::start(const char * ssid, const char * pass, const char * port) {
    if (!init())
        return 1;
    if (!connectToAP(ssid, pass))
        return 2;
    if (!createTCPServer(port))
        return 3;
    return 0;
}


bool ESP8266_HTTP::isHTTP() {
    return isHTTP(msg.message);
}


/**
 * @brief Looks for "HTTP/" in the message.
 * @return true when it is HTTP request.
 */
bool ESP8266_HTTP::isHTTP(const char * message) {
    // GET / HTTP/1.1
    return (strstr(message, "HTTP/") != NULL);
}


/**
 * Checks whether it is HTTP request. Then it takes apart the request. 
 * If the request is not registered, then the response will be NOT FOUND (404), 
 * @return Pointer to Route object which was requested, otherwise NULL
 */
Route * ESP8266_HTTP::preprocessRequest() {
    if (!isHTTP()) {
        // no point sending page404() when it is not even a HTTP request
        closeConnection(msg.channel);
        return NULL;
    }

    // Inspect first line of the message
    char * pCR = strchr(msg.message, '\r');
    pCR[0] = '\0';
    // tokenize first line of the message
    char * pMethod = strtok(msg.message, " ?");
    char * pPath = strtok(NULL, " ?");
    char * pX = strtok(NULL, " ?");

    // Check if the route is registered
    Route *pRoute = isRegistered(pMethod, pPath);
    if (pRoute == NULL) {
        // send Error page
        send404();
        send(msg.channel);
        closeConnection(msg.channel);
        return NULL;
    }

    // Finish Route assembly - check params
    if (isHTTP(pX)) {
        // No params
        // Example: "GET / HTTP/1.1\r\n"
        pRoute->setParams(NULL);
    }
    else {
        // Has params
        // Example: "GET /test?a=5&b=7 HTTP/1.1\r\n"
        pRoute->setParams(pX);
    }
    return pRoute;
}


// Sends generic 404 NOT FOUND response
void ESP8266_HTTP::send404() {
    send_PROGMEM(PROGMEM_HTTP_NOT_FOUND);
}


// Sends generic 200 OK response
void ESP8266_HTTP::send200() {
    send_PROGMEM(PROGMEM_HTTP_OK);
}
