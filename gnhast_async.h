/*!
 * @file gnhast.h
 * Library for connecting to gnhastd as a simple collector
 *
 */

#ifndef __gnhast_async_h__
#define __gnhast_async_h__

/* things we need to setup the whole shebang */

#include <FS.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>

/* need json */
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson

/* Async code */
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncPrinter.h>

/* Update code */
#include <Updater.h>
#include <ESP8266mDNS.h>

/* Wifi Manager bits */
#include <DNSServer.h>
#include <ESPAsyncWiFiManager.h> //https://github.com/alanswx/ESPAsyncWiFiManager/

#ifndef GNHAST_DEBUG
#define GNHAST_DEBUG 0
#endif /*GNHAST_DEBUG*/

#ifndef AP_NAME
#define AP_NAME "Gnhast_AP"
#endif

#ifndef AP_PASSWORD
#define AP_PASSWORD "gnhastyF1"
#endif

#ifndef GNHAST_SERVER_HOST
#define GNHAST_SERVER_HOST "ain.garbled.net"
#endif

/* General library defs */

#define JSON_CONFIG_FILE_SIZE 2048


/*!
 * gnhast defs, like types, subtypes, proto, etc
 */

/* pull in the mainline gnhast.h so we get all the proto defines and whatnot */
#define _GN_ARDUINO_
#include "gnhast_gnhast.h"

/* simplified datatype */
enum gn_dev_datatype {
    DATATYPE_UINT, /**< Unsigned int */
    DATATYPE_DOUBLE, /**< double */
    DATATYPE_LL, /**< uint64_t */
};

/* Hyper simplified from gnhast, we will hack it up hard */
typedef union _data_t {
    uint32_t u;
    double d;
    uint64_t u64;
} gn_data_t;

/*!
 * A device, super simple
 */
struct _gn_dev;
typedef struct _gn_dev {
    char *name;
    char *uid;
    int type;
    int subtype;
    int proto;
    int datatype; /* store the datatype here */
    int scale; /* set scale type here */
    gn_data_t data;
    void *arg; /* pointer that can be used by program, not needed */
} gn_dev_t;

/* Change this if you need more than 10 things. that seems like alot */
#define gn_MAX_DEVICES 10

class gnhast {
 public:
    gnhast(char *coll_name = "ESP", int instance = 1);

    void set_server(char *server, int port);
    void init_server();
    bool connect();
    void disconnect();
    int generic_build_device(char *uid, char *name, int proto, int type, int subtype, int datatype, int scale, void *arg);
    int find_dev_byuid(char *uid);
    void store_data_dev(int dev, gn_data_t data);
    void gn_register_device(int dev);
    void gn_update_device(int dev);
    void set_debug_mode(int mode);
    void imalive();
    void set_collector_health(int health);
    int is_debug();
    gn_dev_t *get_dev_byindex(int idx);

    /* config_helper.cpp */
    DynamicJsonDocument parse_json_conf(char *filename);

    /* wifi_web.cpp */
    void init_wifi();
    boolean init_webserver();
    String getContentType(String filename);
    AsyncWebServer *server;
    AsyncPrinter *ap;

 private:
    int _collector_is_healthy;
    char *_server;
    int _port;
    gn_dev_t _devices[gn_MAX_DEVICES];
    char *_collector_name;
    int _keep_connection;
    int _instance;
    int _nrofdevs;
    int _debug;
    char _gnhast_server[80];
    char _gnhast_port_str[8];
    const char *_dev_argtable[NROF_SUBTYPES] = {
	"none",
	"switch", "outlet", "temp", "humid", "count",
	"pres",	"speed", "dir",	"ph", "wet",
	"hub", "lux", "volts", "wsec", "watt",
	"amps", "rain", "weather", "alarm", "number",
	"pct", "flow", "distance", "volume", "timer",
	"thmode", "thstate", "smnum", "blind", "collector",
	"trigger", "orp", "salinity", "daylight", "moonph",
	"tristate",
    };
    /* web bits */
    bool shouldSaveConfig;
    size_t content_len;

    AsyncClient *client;
    DNSServer dns;

    void __gn_client();
    void __gn_gotdata(void *arg, AsyncPrinter *pri, uint8_t *data, size_t len);

    /* wifi_web.cpp */
    void _read_settings_conf();
    void _save_settings_conf();
    void saveConfigCallback();
    void handleDoUpdate(AsyncWebServerRequest *request,
			const String& filename,
			size_t index, uint8_t *data, size_t len, bool final);
    void handleUpdate(AsyncWebServerRequest *request);
};
    
#endif /*__gnhast_async_h__*/
