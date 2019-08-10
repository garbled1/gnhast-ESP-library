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
    /* zero fill the device table */
    for (i=0; i < gn_MAX_DEVICES; i++) {
	_devices[i].uid = NULL;
	_devices[i].name = NULL;
	_devices[i].type = 0;
	_devices[i].subtype = 0;
	_devices[i].proto = 0;
	_devices[i].arg = NULL;
	_devices[i].datatype = 0;
	_devices[i].data.u = 0;
    }
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

void gnhast::disconnect()
{
    Serial.println("Requesting disconnect from gnhastd");
    if (_client.connected()) {
	_client.println("disconnect");
	_client.stop();
    }
}

void gnhast::imalive()
{
    Serial.println("Telling gnhast we are alive");
    if (_client.connected())
	_client.println("imalive");
}

int gnhast::generic_build_device(char *uid, char *name,
				 int proto, int type, int subtype,
				 int datatype, void *arg)
{
    int i;

    Serial.print("Creating device #");
    Serial.println(i);
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
    _devices[i].arg = arg;

    _nrofdevs++;
    return i;
}

int gnhast::find_dev_byuid(char *uid)
{
    int i;

    for (i=0; i < _nrofdevs; i++)
	if (strcmp(_devices[i].uid, uid) == 0)
	    return i;
    return -1;
}

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

    Serial.println("Registering a device");
    sprintf(buf, "reg uid:%s name:\"%s\" devt:%d subt:%d proto:%d",
	    _devices[dev].uid, _devices[dev].name,
	    _devices[dev].type, _devices[dev].subtype, _devices[dev].proto);
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
