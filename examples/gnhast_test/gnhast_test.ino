/*
  Basic test of gnhast
*/

#include <ESP8266WiFi.h>
#include <gnhast.h>

/* Globals */
char *SSID = "XXXXX";
char *SSID_pass = "XXXXX";
gnhast gnhast("ESP_test", 1);
int fake_sensor;

/* Setup */
void setup() {
    Serial.begin(9600);
    Serial.println();

    Serial.print("Connecting");
    WiFi.begin(SSID, SSID_pass);
    while (WiFi.status() != WL_CONNECTED) {
	delay(500);
	Serial.print(".");
    }
    Serial.println();

    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());

    gnhast.set_server("ain.garbled.net", 2920);
    gnhast.connect();
    delay(200);
    fake_sensor = gnhast.generic_build_device("ESP_faker1", "Fake ESP device",
					      PROTO_SENSOR, DEVICE_SENSOR,
					      SUBTYPE_NUMBER, DATATYPE_UINT,
					      0, NULL);
    if (fake_sensor < 0) {
	Serial.println("Out of devices!!");
	delay(5000);
	/* it's basically hosed at this point */
	return;
    }
    gnhast.gn_register_device(fake_sensor);
}

/* Main loop */
void loop() {
    gn_data_t data;

    /* we use the global fake_sensor, as it's our "device" */

    /* set the uint data to a 5 and store it */
    Serial.println("Storing a 5");
    data.u = 5;
    gnhast.store_data_dev(fake_sensor, data);

    /* write it to gnhast */
    Serial.println("Sending upd to gnhastd");
    gnhast.gn_update_device(fake_sensor);

    /* sleep */
    Serial.println("Sleep for 5 seconds-ish");
    delay(5000);
}