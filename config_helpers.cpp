/*
 * Gnhast config file helper routines
 */

#include "gnhast_async.h"

DynamicJsonDocument gnhast::parse_json_conf(char *filename)
{
    DynamicJsonDocument json_doc(JSON_CONFIG_FILE_SIZE);
    DeserializationError j_error;
    File configFile;
    
    if (_debug)
	Serial.println("mounting FS...");
    if (SPIFFS.begin()) {
	if (_debug) {
	    Serial.println("mounted file system");
	    Serial.printf("Searching for %s\n", filename);
	}

	if (SPIFFS.exists(filename)) {
	    configFile = SPIFFS.open(filename, "r");

	    Serial.printf("reading config file %s\n", filename);
	    if (configFile) {
		if (_debug)
		    Serial.println("opened config file");
		j_error = deserializeJson(json_doc, configFile);
		if (!j_error) {
		    if (_debug) {
			serializeJson(json_doc, Serial);
			Serial.println("\nparsed json");
		    }
		} else {
		    Serial.printf("failed to load json config %s", filename);
		}
		configFile.close();
	    } else
		Serial.println("Cannot open config file for reading");
	} else
	    Serial.println("No config file found");
    } else {
	Serial.println("failed to mount FS");
    }
    return(json_doc);
}

/*
 * Parse the gnhast config
 */

DynamicJsonDocument gnhast::read_gnhast_config()
{
    DynamicJsonDocument doc = parse_json_conf("/gnhast.json");
 
    return(doc);
}

/*
 * Save gnhast config
 */
void gnhast::save_gnhast_config()
{
    DynamicJsonDocument json_doc(JSON_CONFIG_FILE_SIZE);
    gn_dev_t *dev;
    int i;
	
    Serial.println("saving gnhast config");

    for (i=0; i < gn_MAX_DEVICES; i++) {
	dev = get_dev_byindex(i);
	if (dev == NULL)
	    continue;
	json_doc[dev->uid]["name"] = dev->name;
    }

    json_doc["collector_name"] = _collector_name;
    json_doc["instance"] = _instance;
    
    File configFile = SPIFFS.open("/gnhast.json", "w");
    if (!configFile) {
	Serial.println("failed to open gnhast config file for writing");
    }

    if (is_debug()) {
	serializeJson(json_doc, Serial);
	Serial.println();
    }
    serializeJson(json_doc, configFile);
    configFile.close();
}
