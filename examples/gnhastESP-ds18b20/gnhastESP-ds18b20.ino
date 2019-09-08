/*
  An ESP8266 Gnhast collector wired for multiple DS18B20 Chips
*/

/* gnhast and onewire code */
#include <gnhast_async.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/* timer stuff */
extern "C" {
#include <osapi.h>
#include <os_type.h>
}

/* code for the webpage */
#include "webpage.h"

#define ONE_WIRE_BUS 14           /* what pin is our data on ? */
#define JSON_DOC_WRITE_SIZE 2048  /* max size of config files */
#define AP_NAME "Gnhast_AP"
#define AP_PASSWORD "gnhastyF1"   /* Change this if you want */

#define REFRESH_SECONDS 60 /* how fast to read sensor and update gnhast? */
#define TEMPERATURE_PRECISION 12 /* bits of prec in DS18b20 */
#define MAX_BAD_CHECKS 10 /* how many checks before device is considered broken? */

/* Defaults for the config */
#define GNHAST_SERVER_HOST "ain.garbled.net"

/* globals and flags */
static os_timer_t sensor_check_timer;
int devcount;

gnhast gnhast("ESP_onewire", 1);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

/* prototypes */
char *makeOWAddress(uint8_t *da);

/********************* Webserver setup *******************/

/*
 * Handle the / request.  Generate values and fill it in
 */

void handleRoot(AsyncWebServerRequest *request)
{
    int i;
    gn_dev_t *dev;
    char buf[50];
    String devline(device_line);
    AsyncResponseStream *response = request->beginResponseStream("text/html");

    response->addHeader("Server","ESP Gnhast");
    response->print(gnhast_header);
    for (i=0; i < gn_MAX_DEVICES; i++) {
    	dev = gnhast.get_dev_byindex(i);
    	if (dev == NULL)
    	    continue;
    	String copy = devline;
    	copy.replace("DEVUID", dev->uid);
    	copy.replace("CURNAME", dev->name);
    	snprintf(buf, 50, "%f", dev->data.d);
    	copy.replace("DEVVALUE", buf);
	response->print(copy.c_str());
    }
    response->print(gnhast_footer);
    request->send(response);
}

/************** Sensor code goes here ****************/

/*
  Function to make an ow address string and return as strdup'd char *
*/
char *makeOWAddress(uint8_t *da)
{
    char buf[64];
    
    sprintf(buf, "%0.2X%0.2X%0.2X%0.2X%0.2X%0.2X%0.2X%0.2X",
	    da[0], da[1], da[2], da[3],
	    da[4], da[5], da[6], da[7]);
    return(strdup(buf));
}

/* Loop through devices found and create them */

int create_devices(int nrofdevs)
{
    int i, created=0, haveconfig=0;
    DeviceAddress d_addr;
    uint8_t *dp;
    char devname[80];
    char *uid;

    /* first read the config file */
    DynamicJsonDocument gncfg = gnhast.read_gnhast_config();
    serializeJson(gncfg, Serial);
    Serial.println();
    
    for (i=0; i < nrofdevs && i < gn_MAX_DEVICES; i++){
	if (!sensors.getAddress(d_addr, i))
	    continue;
	uid = makeOWAddress(d_addr);
	const char *dname = gncfg[uid]["name"];
	if (dname)
	    snprintf(devname, 80, "%s", dname);
	else
	    snprintf(devname, 80, "ESP DS18B20 dev #%d", i);
	/* copy devaddr to a pointer */
	dp = (uint8_t *)malloc(sizeof(uint8_t)*8);
	memcpy(dp, &d_addr, 8);
	Serial.printf("DEV #%d - creating uid:%s name:%s\n", i, uid, devname);
	if (gnhast.generic_build_device(uid, devname,
					PROTO_SENSOR_INDOOR, DEVICE_SENSOR,
					SUBTYPE_TEMP, DATATYPE_DOUBLE,
					0, dp) < 0) {
	    Serial.println("Cannot create device");
	    continue;
	}
	gnhast.gn_register_device(i);
	sensors.setResolution(d_addr, TEMPERATURE_PRECISION);
	created++;
    }
    if (gnhast.is_debug())
	Serial.printf("Created %d of %d devices found\n", created, i);
    if (gncfg.isNull())
	gnhast.save_gnhast_config();
    return(created);
}

/*
 * A routine to check the sensors, called periodically by a timer
 * This is a bit of a hack for now, because calling the ap->print function
 * repeatedly, fast, seems to knock it over.
 */

static void check_sensors(void *arg)
{
    static int i;
    static int bad_checks;
    float f;
    gn_data_t data;
    gn_dev_t *dev;
    DeviceAddress d_addr;

    /* read the sensors */
    sensors.requestTemperatures();

    if (i == gn_MAX_DEVICES || i == devcount)
	i = 0;

    dev = NULL;
    for (i; i < gn_MAX_DEVICES && i < devcount; i++) {
        dev = gnhast.get_dev_byindex(i);
	if (dev == NULL)
	    continue;
	break;
    }

    if (dev == NULL)
	return;

    Serial.printf("Working device #%d: ", i);
    memcpy(&d_addr, dev->arg, 8);
    f = sensors.getTempF(d_addr);
    if (f > DEVICE_DISCONNECTED_F) {
	data.d = f;
	Serial.println(data.d);
	gnhast.store_data_dev(i, data);
	gnhast.gn_update_device(i);
	bad_checks = 0; /* got good data */
	gnhast.set_collector_health(1);
    } else {
	Serial.printf("Device disconnected!\n");
	bad_checks++;
	if (bad_checks > MAX_BAD_CHECKS)
	    gnhast.set_collector_health(0);
    }

    i++;
}

/* 
 * Setup
 */
void setup() {
    int i;
    DeviceAddress d_addr;
    
    Serial.begin(115200);
    Serial.println();
    gnhast.set_debug_mode(0);

    /* turn on 1wire */
    sensors.begin();
    /* read the sensors once so the webpage has data */
    sensors.requestTemperatures();
    /* setup the wifi */
    gnhast.init_wifi();

    /* fire up the updates aware webserver, and set a homepage */
    gnhast.init_webserver();
    gnhast.server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){handleRoot(request);});

    /* connect to gnhast */
    gnhast.init_server();
    gnhast.connect();

    devcount = create_devices(sensors.getDeviceCount());

    os_timer_setfn(&sensor_check_timer, &check_sensors, NULL);
    os_timer_arm(&sensor_check_timer, (REFRESH_SECONDS * 1000)/devcount, true);
}

/* Main loop, should be left alone. */
void loop() {
    if (gnhast.shouldReboot) {
	delay(100);
	ESP.restart();
    }
}
