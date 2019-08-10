/*!
 * @file gnhast.h
 * Library for connecting to gnhastd as a simple collector
 *
 */

#ifndef __gnhast_h__
#define __gnhast_h__

#include <Arduino.h>
#include <ESP8266WiFi.h>

/*!
 * gnhast defs, like types, subtypes, proto, etc
 */

/** Basic device types */
/** \note a type blind should always return BLIND_STOP, for consistency */
enum DEV_TYPES {
    DEVICE_NONE,        /**< No device, don't use */
    DEVICE_SWITCH,      /**< A switch on/off device */
    DEVICE_DIMMER,      /**< A dimmer */
    DEVICE_SENSOR,      /**< A device that is read-only */
    DEVICE_TIMER,       /**< A timer */
    DEVICE_BLIND,       /**< can go up or down, but cannot be queried */
    NROF_TYPES,         /**< Used for looping, never as a type */
};

enum PROTO_TYPES {
    PROTO_NONE, /**< 0 No protocol */
    PROTO_INSTEON_V1, /**< 1 A version 1 insteon device */
    PROTO_INSTEON_V2, /**< 2 A version 2 insteon device */
    PROTO_INSTEON_V2CS, /**< 3 A version 2 insteon device with checksum */
    PROTO_SENSOR_OWFS, /**< 4 An OWFS presented device */
    PROTO_SENSOR_BRULTECH_GEM, /**< 5 A brultech GEM */
    PROTO_SENSOR_BRULTECH_ECM1240, /**< 6 A brultech ECM1240 */
    PROTO_SENSOR_WMR918, /**< 7 A WMR918 weather station */
    PROTO_SENSOR_AD2USB, /**< 8 An ad2usb ademco alarm integration */
    PROTO_SENSOR_ICADDY, /**< 9 Irrigation caddy */
    PROTO_SENSOR_VENSTAR, /**< 10 Venstar Thermostat */
    PROTO_CONTROL_URTSI, /**< 11 Somfy URTSI blind controller */
    PROTO_COLLECTOR, /**< 12 A generic collector */
    PROTO_CAMERA_AXIS, /**< 13 An AXIS camera */
    PROTO_TUXEDO, /**< 14 Honeywell Tuxedo Touch */
    PROTO_NEPTUNE_APEX, /**< 15 Neptune Apex aquarium controller */
    PROTO_CALCULATED, /**< 16 for calculated values */
    PROTO_BALBOA, /**< 17 A balboa spa wifi controller */
    PROTO_GENERIC, /**< 18 A Generic catch-all */
    PROTO_UNUSED1,
    PROTO_UNUSED2, /*20*/
    PROTO_UNUSED3,
    PROTO_UNUSED4,
    PROTO_UNUSED5,
    PROTO_UNUSED6,
    PROTO_UNUSED7,
    PROTO_UNUSED8,
    PROTO_UNUSED9,
    PROTO_UNUSED10, /*28*/
    PROTO_UNUSED11,
    PROTO_UNUSED12,
    PROTO_UNUSED13,
    PROTO_UNUSED14, /*32*/
    /* At this point, starting over, with use based protocols */
    PROTO_MAINS_SWITCH, /**< 33 A mains switch/outlet/otherwise */
    PROTO_WEATHER, /**< 34 Weather data */
    PROTO_SENSOR, /**< 35 Any random sensor */
    PROTO_SENSOR_INDOOR, /**< 36 A sensor for indoor data collection */
    PROTO_SENSOR_OUTDOOR, /**< 37 A sensor for outdoor data collection */
    PROTO_SENSOR_ELEC, /**< 38 A sensor that monitors electronics */
    PROTO_POWERUSE, /**< 39 Power usage data */
    PROTO_IRRIGATION, /**< 40 Irrigation data */
    PROTO_ENTRY, /**< 41 entry/egress, like a door, window, etc */
    PROTO_ALARM, /**< 42 An alarm of some kind */
    PROTO_BATTERY, /**< 43 Battery monitor */
    PROTO_CAMERA, /**< 44 A camera */
    PROTO_AQUARIUM, /**< 45 Aquarium stuff */
    PROTO_SOLAR, /**< 46 Solar power */
    PROTO_POOL, /**< 47 Pools, spas */
    PROTO_WATERHEATER, /**< 48 Water heaters */
    PROTO_LIGHT, /**< 49 Lights (bulbs, leds, whatever) */
    PROTO_AV, /**< 50 Audio/video equipment */
    PROTO_THERMOSTAT, /**< 51 Thermostat data */
    PROTO_SETTINGS, /**< 52 Device settings/config */
    PROTO_BLIND, /**< 53 Blinds, shutters, etc */
    NROF_PROTOS,
};
/* Update when adding a new protocol */
#define PROTO_MAX PROTO_BLIND

enum SUBTYPE_TYPES {
    SUBTYPE_NONE,
    SUBTYPE_SWITCH,
    SUBTYPE_OUTLET,
    SUBTYPE_TEMP,
    SUBTYPE_HUMID,
    SUBTYPE_COUNTER,
    SUBTYPE_PRESSURE,
    SUBTYPE_SPEED,
    SUBTYPE_DIR,
    SUBTYPE_PH, /* stored in ph */
    SUBTYPE_WETNESS,
    SUBTYPE_HUB,
    SUBTYPE_LUX,
    SUBTYPE_VOLTAGE,
    SUBTYPE_WATTSEC,
    SUBTYPE_WATT,
    SUBTYPE_AMPS,
    SUBTYPE_RAINRATE,
    SUBTYPE_WEATHER, /* stored in state */
    SUBTYPE_ALARMSTATUS, /* stored in state */
    SUBTYPE_NUMBER,
    SUBTYPE_PERCENTAGE, /* stored in humid */
    SUBTYPE_FLOWRATE,
    SUBTYPE_DISTANCE, 
    SUBTYPE_VOLUME,
    SUBTYPE_TIMER, /* stored in count, used as countdown to zero */
    SUBTYPE_THMODE, /* stored in state */
    SUBTYPE_THSTATE, /* stored in state */
    SUBTYPE_SMNUMBER, /* 8bit number stored in state */
    SUBTYPE_BLIND, /* stored in state, see BLIND_* */
    SUBTYPE_COLLECTOR, /* stored in state */
    SUBTYPE_TRIGGER, /* momentary switch (ui) */
    SUBTYPE_ORP, /* Oxidation Redux Potential (d) */
    SUBTYPE_SALINITY, /* Salinity (d) */
    SUBTYPE_DAYLIGHT, /* daylight (8 bit number stored in state) */
    SUBTYPE_MOONPH, /* lunar phase (d) */
    SUBTYPE_TRISTATE, /* tri-state device 0,1,2 stored in state) */
    SUBTYPE_BOOL, /* Never actually use this one, just for submax */
    NROF_SUBTYPES,
};
#define SUBTYPE_MAX SUBTYPE_BOOL

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

 private:
    char *_server;
    int _port;
    gn_dev_t _devices[gn_MAX_DEVICES];
    char *_collector_name;
    int _keep_connection;
    int _instance;
    int _nrofdevs;
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
