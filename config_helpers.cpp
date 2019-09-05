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
