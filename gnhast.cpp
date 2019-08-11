/*!
 * @file gnhast.cpp
 * @mainpage gnhast collector library for Arduino/ESP
 * @section intro_sec Introduction
 *
 * Very basic gnhast collector library.
 * Support for:
 * connect to gnhastd
 * send reg
 * send upd
 *
 *
 * @section author Author
 * Tim Rightnour
 *
 * @section license License
 *
 *  BSD license, all text above must be included in any redistribution
 */

#include "Arduino.h"
#include "gnhast.h"

/*!
 * @brief Instantiates a new gnhast class
 * @param *coll_name
          Name of collector
*/
gnhast::gnhast(char *coll_name, int instance) {
    int i;
    
    _server = "gnhastd";
    _port = 2920;
    _collector_name = coll_name;
    _keep_connection = 0;
    _instance = instance;
    _nrofdevs = 0;
    _debug = GNHAST_DEBUG;
    /* zero fill the device table */
    for (i=0; i < gn_MAX_DEVICES; i++) {
	_devices[i].uid = NULL;
	_devices[i].name = NULL;
	_devices[i].type = 0;
	_devices[i].subtype = 0;
	_devices[i].proto = 0;
	_devices[i].scale = 0;
	_devices[i].arg = NULL;
	_devices[i].datatype = 0;
	_devices[i].data.u = 0;
    }
}

/*!
 * @brief Set debug mode
 */
void gnhast::set_debug_mode(int mode)
{
    _debug = mode;
    if (_debug)
	Serial.println("Turned debug mode ON");
    else
	Serial.println("Turned debug mode OFF");
}

/*!
 * @brief Set server and port
 */

void gnhast::set_server(char *server, int port)
{
    _server = strdup(server);
    _port = port;
}

/*!
 * @brief Connect to gnhastd, initiate collector
 */

bool gnhast::connect()
{
    char buf[256];
    
    Serial.print("Connecting to: ");
    Serial.print(_server);
    Serial.print(":");
    Serial.println(_port);

    if (!_client.connect(_server, _port)) {
	Serial.println("Connection failed!");
	delay(1000);
	return false;
    }
    sprintf(buf, "client client:%s-%0.3d", _collector_name, _instance);
    if (_client.connected()) {
	_client.println(buf);
    }
    return true;
}

/*!
 * @brief Disconnect from gnhast nicely
 */

void gnhast::disconnect()
{
    if (_debug)
	Serial.println("Requesting disconnect from gnhastd");
    if (_client.connected()) {
	_client.println("disconnect");
	_client.stop();
    }
}

/*!
 * @brief Tell gnhast we are still alive
 */

void gnhast::imalive()
{
    if (_debug)
	Serial.println("Telling gnhast we are alive");
    if (_client.connected())
	_client.println("imalive");
}

/*!
 * @brief build a device that can later be registered.
 * Unlike normal gnhast, we just have an integer array of devices, to keep
 * it simple and small.  Also, the device definitions are greatly reduced, for
 * simplicity.
 */

int gnhast::generic_build_device(char *uid, char *name,
				 int proto, int type, int subtype,
				 int datatype, int scale, void *arg)
{
    int i;

    if (_debug) {
	Serial.print("Creating device #");
	Serial.println(i);
    }
    i = _nrofdevs;
    if (i == gn_MAX_DEVICES) {
	Serial.println("Too many devices in generic_build_device, increase gn_MAX_DEVICES and rebuild");
	return -1;
    }

    if (NULL == uid || NULL == name) {
	Serial.println("NULL name/uid in generic_build_device, punt");
    }
    
    _devices[i].uid = strdup(uid);
    _devices[i].name = strdup(name);
    _devices[i].type = type;
    _devices[i].proto = proto;
    _devices[i].subtype = subtype;
    _devices[i].datatype = datatype;
    _devices[i].scale = scale;
    _devices[i].arg = arg;

    _nrofdevs++;
    return i;
}

/*!
 * @brief Find a device by it's uid.  linear search.  only 10 devices, so, ok.
 */

int gnhast::find_dev_byuid(char *uid)
{
    int i;

    for (i=0; i < _nrofdevs; i++)
	if (strcmp(_devices[i].uid, uid) == 0)
	    return i;
    return -1;
}

/*!
 * @brief store a datapoint in a device for later upd
 */

void gnhast::store_data_dev(int dev, gn_data_t data)
{
    switch (_devices[dev].datatype) {
    case DATATYPE_UINT:
	_devices[dev].data.u = data.u;
	break;
    case DATATYPE_DOUBLE:
	_devices[dev].data.d = data.d;
	break;
    case DATATYPE_LL:
	_devices[dev].data.u64 = data.u64;
    }
}

/*!
 * @brief register a device with gnhast
 */

void gnhast::gn_register_device(int dev)
{
    char buf[1024];

    /* Sanity verification */
    if (NULL == _devices[dev].name || NULL == _devices[dev].uid ||
	_devices[dev].type == 0 || _devices[dev].proto == 0 ||
	_devices[dev].subtype == 0) {
	Serial.print("Device #");
	Serial.print(dev);
	Serial.println(" is badly formed, cannot register");
	return;
    }

    if (_debug)
	Serial.println("Registering a device");

    sprintf(buf, "reg uid:%s name:\"%s\" devt:%d subt:%d proto:%d scale:%d",
	    _devices[dev].uid, _devices[dev].name,
	    _devices[dev].type, _devices[dev].subtype,
	    _devices[dev].proto, _devices[dev].scale);

    if (_debug)
	Serial.println(buf);
    
    if (!_client.connected()) {
	if (!connect()) {
	    Serial.println("register cannot connect to gnhastd!");
	    return;
	}
    }
    _client.println(buf);
    return;
}

/*!
 * @brief update the infor for a device, specifically the data.
 */

void gnhast::gn_update_device(int dev)
{
    char buf[1024];

    /* Sanity verification */
    if (NULL == _devices[dev].name || NULL == _devices[dev].uid ||
	_devices[dev].type == 0 || _devices[dev].proto == 0 ||
	_devices[dev].subtype == 0) {
	Serial.print("Device #");
	Serial.print(dev);
	Serial.println(" is badly formed, cannot update");
	return;
    }

    if (_debug)
	Serial.println("Doing an update");
    
    switch (_devices[dev].datatype) {
    case DATATYPE_UINT:
	sprintf(buf, "upd uid:%s %s:%d",
		_devices[dev].uid,
		_dev_argtable[_devices[dev].subtype],
		_devices[dev].data.u);
	break;
    case DATATYPE_DOUBLE:
	if (_devices[dev].type == DEVICE_DIMMER) {
	    sprintf(buf, "upd uid:%s dimmer:%f",
		_devices[dev].uid,
		_devices[dev].data.d);
	} else {
	    sprintf(buf, "upd uid:%s %s:%f",
		    _devices[dev].uid,
		    _dev_argtable[_devices[dev].subtype],
		    _devices[dev].data.d);
	}
	break;
    case DATATYPE_LL:
	sprintf(buf, "upd uid:%s %s:%jd",
		_devices[dev].uid,
		_dev_argtable[_devices[dev].subtype],
		_devices[dev].data.u64);
	break;
    }
    if (_debug)
	Serial.println(buf);

    if (!_client.connected()) {
	if (!connect()) {
	    Serial.println("update cannot connect to gnhastd!");
	    return;
	}
    }
    _client.println(buf);
    return;
}
