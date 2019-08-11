/*!
 * @file gnhast.h
 * Library for connecting to gnhastd as a simple collector
 *
 */

#ifndef __gnhast_h__
#define __gnhast_h__

#include <Arduino.h>
#include <ESP8266WiFi.h>

#ifndef GNHAST_DEBUG
#define GNHAST_DEBUG 0
#endif /*GNHAST_DEBUG*/

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
    gn_data_t data;
    void *arg; /* pointer that can be used by program, not needed */
} gn_dev_t;
/* Change this if you need more than 10 things. that seems like alot */
#define gn_MAX_DEVICES 10

class gnhast {
 public:
    gnhast(char *coll_name = "ESP", int instance = 1);

    void set_server(char *server, int port);
    bool connect();
    void disconnect();
    void imalive();
    int generic_build_device(char *uid, char *name, int proto, int type, int subtype, int datatype, void *arg);
    int find_dev_byuid(char *uid);
    void store_data_dev(int dev, gn_data_t data);
    void gn_register_device(int dev);
    void gn_update_device(int dev);
    void set_debug_mode(int mode);

 private:
    char *_server;
    int _port;
    gn_dev_t _devices[gn_MAX_DEVICES];
    char *_collector_name;
    int _keep_connection;
    int _instance;
    int _nrofdevs;
    int _debug;
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
    WiFiClient _client;
};
    
#endif /*__gnhast_h__*/
