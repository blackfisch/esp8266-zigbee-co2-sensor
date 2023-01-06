/*
  MQUnifiedsensor Library - reading an MQ135

  Demonstrates the use a MQ135 sensor.
  Library originally added 01 may 2019
  by Miguel A Califa, Yersson Carrillo, Ghiordy Contreras, Mario Rodriguez

  Added example
  modified 23 May 2019
  by Miguel Califa

  Updated library usage
  modified 26 March 2020
  by Miguel Califa

  Wiring:
  https://github.com/miguel5612/MQSensorsLib_Docs/blob/master/static/img/MQ_Arduino.PNG
  Please make sure arduino A0 pin represents the analog input configured on #define pin

 This example code is in the public domain.
*/

// Include the library
#include <RGBLed.h>
#include <DHT.h>
#include <MQ135.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Definitions
#define placa "ESP8266"
#define Voltage_Resolution 3.3
#define pin A0                 // Analog input 0 of your arduino
#define type "MQ-135"          // MQ135
#define ADC_Bit_Resolution 10  // For arduino UNO/MEGA/NANO
#define RatioMQ135CleanAir 4.4 // RS / R0 = 3.6 ppm  // Datasheet = 4.4
#define R0_AMBIENT 60//125

#define DHTTYPE DHT11
#define DHTPIN D5 // SDD3 Pin

#define PIN_RED D1   // pin that red led is connected to
#define PIN_GREEN D2 // pin that green led is connected to
#define PIN_BLUE D3  // pin that blue led is connected to

// Declare Sensor
MQ135 gasSensor = MQ135(pin, R0_AMBIENT);
RGBLed led(PIN_RED, PIN_GREEN, PIN_BLUE, RGBLed::COMMON_ANODE);
DHT dht(DHTPIN, DHTTYPE);

/*
  MQTT Setup
  Reference: https://github.com/smrtnt/Open-Home-Automation/blob/master/ha_mqtt_sensor_dht22/ha_mqtt_sensor_dht22.ino
*/
#define MQTT_VERSION MQTT_VERSION_3_1_1

#include "secrets.h" //wifi access
const PROGMEM char *MQTT_CLIENT_ID = "co2_sensor_schreibtisch";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;

// MQTT: topic
const PROGMEM char *MQTT_SENSOR_TOPIC = "schreibtisch/co2sensor";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

struct HT
{
  float h;
  float t;
};

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("INFO: Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("INFO: connected");
    } else {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.println("DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  // Init the serial port communication - to debug the library
  Serial.begin(9600); // Init serial port

  led.off();
  led.brightness(15);

  dht.begin();

  /* WiFi + MQTT */

  Serial.println();
  Serial.print("INFO: Connecting to ");
  WiFi.mode(WIFI_STA);
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.println("INFO: IP address: ");
  Serial.println(WiFi.localIP());

  // init the MQTT connection
  client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  client.setCallback(callback);
}

#define NUM_READINGS 10
float vals[NUM_READINGS];
int i = 0;
float average = -1;

#define DELAY 1000

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  HT ht = read_dht();
  float hic = dht.computeHeatIndex(ht.t, ht.h, false);
  vals[i] = gasSensor.getCorrectedPPM(ht.t, ht.h);

  /*
  Serial.print("Humidity: ");
  Serial.print(ht.h);
  Serial.print(" | Temperature: ");
  Serial.print(ht.t);
  Serial.print("°C | Head Index: ");
  Serial.print(hic);
  Serial.print("°C | PPM CO2: ");
  Serial.println(vals[i]);
  */

  i++;
  if (i == NUM_READINGS)
  {
    i = 0;
    average = calc_average();
    publishData(ht, hic, average);
  }

  // MQ135.serialDebug();
  update_display(average);

  delay(DELAY);
}

HT read_dht()
{
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return {h, t};
  }

  return {h, t};
}

void update_display(float val)
{
  if (val >= 2000)
  {
    led.flash(RGBLed::RED, DELAY / 2, DELAY);
  }
  else if (val > 1400)
  {
    led.setColor(RGBLed::RED);
  }
  else if (val > 1000)
  {
    led.setColor(255, 165, 0); // orange
  }
  else if (val > 800)
  {
    led.setColor(RGBLed::YELLOW);
  }
  else
  {
    led.setColor(RGBLed::GREEN);
  }
}

float calc_average()
{
  float sum = 0;
  for (int i = 0; i < NUM_READINGS; i++)
  {
    sum += vals[i];
  }
  return sum / NUM_READINGS;
}

void publishData(HT p_heat_temp, float p_heat_index, float p_co2)
{
  // create a JSON object
  // doc : https://github.com/bblanchon/ArduinoJson/wiki/API%20Reference
  StaticJsonDocument<200> jsonBuffer;
  JsonObject root = jsonBuffer.to<JsonObject>();
  // INFO: the data must be converted into a string; a problem occurs when using floats...
  root["temperature"] = (String)p_heat_temp.t;
  root["humidity"] = (String)p_heat_temp.h;
  root["heatIndex"] = (String)p_heat_index;
  root["co2"] = (String)p_co2;
  serializeJsonPretty(root, Serial);
  Serial.println("");
  char data[200];
  serializeJson(root, data, measureJson(root) + 1);
  client.publish(MQTT_SENSOR_TOPIC, data, true);
  yield();
}

// function called when a MQTT message arrived
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
  Serial.println(" ------------------- ");
  Serial.println("Incoming MQTT Message");
  //Serial.println(p_topic);
  Serial.println(" ------------------- ");
}