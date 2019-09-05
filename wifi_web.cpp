/*
 * Wifi setup and webpage serving
 */

#include "gnhast_async.h"

/* read the settings file */

void gnhast::_read_settings_conf()
{
    DynamicJsonDocument json_doc = parse_json_conf("/config.json");

    if (json_doc["gnhast_server"] && json_doc["gnhast_port"]) {
	strcpy(_gnhast_server, json_doc["gnhast_server"]);
	strcpy(_gnhast_port_str, json_doc["gnhast_port"]);
	Serial.printf("From config /config.json:\nGnhast Server: %s:%s\n",
		      _gnhast_server, _gnhast_port_str);
    }
}

/*
 * Save the basic settings
 */

void gnhast::_save_settings_conf()
{
    DynamicJsonDocument json_doc(JSON_CONFIG_FILE_SIZE);

    Serial.println("saving config");
    json_doc["gnhast_server"] = _gnhast_server;
    json_doc["gnhast_port"] = _gnhast_port_str;
    
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
	Serial.println("failed to open config file for writing");
    }

    if (_debug)
	serializeJson(json_doc, Serial);
    serializeJson(json_doc, configFile);
    configFile.close();
}

/* callback telling us to save the config */
void gnhast::saveConfigCallback()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

/*
 * Setup the wifi
 */

void gnhast::init_wifi()
{
    /* Things that we will ask the user for */
    AsyncWiFiManagerParameter custom_gnhast_server("gn_server", "Gnhast Server",
						   _gnhast_server, 79);
    AsyncWiFiManagerParameter custom_gnhast_port("gn_port", "Gnahst port",
						 _gnhast_port_str, 7);

    server = new AsyncWebServer(80);
    AsyncWiFiManager wifiManager(server, &dns);

    /* read our config file */
    _read_settings_conf();
    
    /* debug only */
    //wifiManager.resetSettings();
    //wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setDebugOutput(true);

    /* defaults to 8% */
    wifiManager.setMinimumSignalQuality();

    wifiManager.addParameter(&custom_gnhast_server);
    wifiManager.addParameter(&custom_gnhast_port);

    /* If we have a configured SSID/etc, it will connect, otherwise, start
       an AP */
    if (!wifiManager.autoConnect(AP_NAME, AP_PASSWORD)) {
	Serial.println("Failed to connect and hit timeout");
	delay(6000);
	ESP.reset(); /* woo, that's harsh */
	delay(5000);
    }
    /* read the gnhast settings */
    strcpy(_gnhast_server, custom_gnhast_server.getValue());
    strcpy(_gnhast_port_str, custom_gnhast_port.getValue());
    _port = atoi(_gnhast_port_str);

    /* Let's save these to disk */
    if (shouldSaveConfig)
	_save_settings_conf();

    Serial.println("Wifi is connected");

    /* tell the serial port we are all wired up */
    Serial.println("local ip");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.gatewayIP());
    Serial.println(WiFi.subnetMask());
}

/********************  Over the air update code ********************/

/* 
   Updater request

   This provides a simple form to allow uploading of new code to the
   ESP.
*/
void gnhast::handleUpdate(AsyncWebServerRequest *request)
{
    char* html = "<form method='POST' action='/doUpdate' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
    request->send(200, "text/html", html);
}

/*
  This handles the actual update.  We are uploaded a binary, and
  reflash ourselves with it.
*/

void gnhast::handleDoUpdate(AsyncWebServerRequest *request,
		    const String& filename,
		    size_t index, uint8_t *data, size_t len, bool final)
{
    if (!index){
	Serial.println("Update");
	content_len = request->contentLength();
	// if filename includes spiffs, update the spiffs partition
	int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
	Update.runAsync(true);
	if (!Update.begin(content_len, cmd)) {
	    Update.printError(Serial);
	}
    }

    if (Update.write(data, len) != len) {
	Update.printError(Serial);
    } else {
	Serial.printf("Progress: %d%%\n", (Update.progress()*100)/Update.size());
    }

    if (final) {
	AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
	response->addHeader("Refresh", "20");  
	response->addHeader("Location", "/");
	request->send(response);
	if (!Update.end(true)){
	    Update.printError(Serial);
	} else {
	    Serial.println("Update complete");
	    Serial.flush();
	    ESP.restart();
	}
    }
}

/* File type handler */

String gnhast::getContentType(String filename)
{
    if (filename.endsWith(".htm")) return "text/html";
    else if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".xml")) return "text/xml";
    else if (filename.endsWith(".pdf")) return "application/x-pdf";
    else if (filename.endsWith(".zip")) return "application/x-zip";
    else if (filename.endsWith(".gz")) return "application/x-gzip";
    return "text/plain";
}

/*
 * Setup the internal webserver
 */

boolean gnhast::init_webserver()
{
    server->on("/update", HTTP_GET, [this](AsyncWebServerRequest *request){handleUpdate(request);});
    server->on("/doUpdate", HTTP_POST,
	      [this](AsyncWebServerRequest *request) {},
	      [this](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,
		 size_t len, bool final) {handleDoUpdate(request, filename, index, data, len, final);}
  );
    server->onNotFound([this](AsyncWebServerRequest *request) {
	    String ct = getContentType(request->url());
	    if (SPIFFS.exists(request->url()))
		request->send(SPIFFS, request->url(), ct);
	    else
		request->send(404, "text/plain", "404: Not Found");
	});
    server->begin();
}
