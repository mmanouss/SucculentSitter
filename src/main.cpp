#include <Arduino.h>
#include <ArduinoJson.h>
#include <HttpClient.h>
#include <WiFi.h>
#include <inttypes.h>
#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "DHT20.h"
#include <TFT_eSPI.h>

#define BUZZER_PIN 15
#define PHOTORESISTOR_PIN 33

// DEFINING BUZZER VALUES
#define BUZZER_OFF_STATE 0
#define BUZZER_ON_STATE 1
#define BUZZER_OFF_TIME 1800000 // 30min * 60s/min = 1800s * 1000ms/s = 1800000ms
#define BUZZER_ON_TIME 10000 // 10s * 1000ms/s = 10000ms

struct SensorData {
    String temp;
    String moisture;
    String light;
};

int buzzer_state; // Buzzer state
unsigned long buzzer_timer; // Buzzer timer

TFT_eSPI ttg = TFT_eSPI(); 
void display_loop(SensorData sensor_val);

// Server details
const char serverAddress[] = "3.149.230.7"; // adjust with instance
const int serverPort = 5000;

// This example downloads the URL "http://arduino.cc/"
char ssid[50]; // your network SSID (name)
char pass[50]; // your network password (use for WPA, or use as key for WEP)

const char kHostname[] = "worldtimeapi.org"; // Name of the server we want to connect to
const char kPath[] = "/api/timezone/Europe/London.txt"; // Path to download (this is the bit after the hostname in the URL that you want to download

const int kNetworkTimeout = 30 * 1000; // num of ms to wait without receiving any data before we give up
const int kNetworkDelay = 1000; // num of ms to wait if no data is available before trying again

// Photoresistor mapping
const int lightMin = 0;
const int lightMax = 4095;

const int desiredMin = 0;
const int desiredMax = 100;

// DHT initialization
DHT20 DHT;
uint8_t count_var = 0;
const int dry = 60; // this number and below indicates a dry plant
const int shade = 2500; // this number and below indicates not in a sunny spot

// Function declarations
void nvs_access();
void aws_setup();
void aws_loop(const String & send_val);
String aws_loop_msg(SensorData sensor_val);

SensorData temp_moisture_light();
void sensor_data_setup();
SensorData sensor_data_loop();

void nvs_access() 
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    // Open
    Serial.printf("\n");
    Serial.printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        Serial.printf("Done\n");
        Serial.printf("Retrieving SSID/PASSWD\n");
        size_t ssid_len;
        size_t pass_len;
        err = nvs_get_str(my_handle, "ssid", ssid, &ssid_len);
        err |= nvs_get_str(my_handle, "pass", pass, &pass_len);
        switch (err) {
            case ESP_OK:
                Serial.printf("Done\n");
                //Serial.printf("SSID = %s\n", ssid);
                //Serial.printf("PASSWD = %s\n", pass);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                Serial.printf("The value is not initialized yet!\n");
                break;
            default:
                Serial.printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
    }
    // Close
    nvs_close(my_handle);
}

void aws_setup()
{
    // Retrieve SSID/PASSWD from flash before anything else
    nvs_access();
    
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("MAC address: ");
    Serial.println(WiFi.macAddress());
}

String aws_loop_msg(SensorData sensor_val)
{
  float f = (sensor_val.temp.toFloat() * 1.8) + 32;
  int mappedValue = map(sensor_val.light.toFloat(), lightMin, lightMax, desiredMin, desiredMax);

  if (sensor_val.moisture.toFloat() >= dry && sensor_val.light.toFloat() >= shade)
    return "Temperature: " + sensor_val.temp + "°C / " + f + "°F, Moisture: " + sensor_val.moisture + "%, Light: " + String(mappedValue) + "%";
  else if (sensor_val.moisture.toFloat() < dry && sensor_val.light.toFloat() < shade)
    return "Temperature: " + sensor_val.temp + "°C / " + f + "°F, Low Moisture: " + sensor_val.moisture + "%, Low Light: " + String(mappedValue) + "%";
  else if (sensor_val.moisture.toFloat() < dry)
    return "Temperature: " + sensor_val.temp + "°C / " + f + "°F, Low Moisture: " + sensor_val.moisture + "%, Light: " + String(mappedValue) + "%";
  else if (sensor_val.light.toFloat() < shade)
    return "Temperature: " + sensor_val.temp + "°C / " + f + "°F, Moisture: " + sensor_val.moisture + "%, Low Light: " + String(mappedValue) + "%";
  return "";
}

void aws_loop(const String & send_val)
{
    int err = 0;
    WiFiClient c;
    HttpClient http(c);
    //err = http.get(kHostname, kPath); // UNCOMMENT WHEN TESTING, COMMENT BELOW LINE

    JsonDocument doc;
    doc["send_val"] = send_val;
    
    String jsonStr;
    serializeJson(doc, jsonStr);

    String path = "/submit";
    http.beginRequest();
    http.post(serverAddress, serverPort, path.c_str());
    http.sendHeader("Content-Type", "application/json");
    http.sendHeader("Content-Length", jsonStr.length());

    http.print(jsonStr);
    http.endRequest();

    // Handle the response from the server
    err = http.responseStatusCode();
    if (err != 200) 
    {
        Serial.print("Got status code: ");
        Serial.println(err);
    }

    http.stop();
}

SensorData temp_moisture_light() 
{
    float t = DHT.getTemperature();
    float h = DHT.getHumidity();
    int l = analogRead(PHOTORESISTOR_PIN);

    SensorData data;
    data.temp = String(t, 2);
    data.moisture = String(h, 2);
    data.light = String(l);

    return data;
}

void sensor_data_setup()
{
  Wire.begin();
  DHT.begin(); // ESP32 default pins 21 22
}

SensorData sensor_data_loop()
{
    SensorData data = temp_moisture_light();
    if (millis() - DHT.lastRead() >= 1000)
    {
      //  READ DATA
      uint32_t start = micros();
      int status = DHT.read();
      uint32_t stop = micros();

      if ((count_var % 10) == 0)
        count_var = 0;
      count_var++;

      switch (status)
      {
        case DHT20_OK:
          break;
        case DHT20_ERROR_CHECKSUM:
          Serial.println("Checksum error");
          break;
        case DHT20_ERROR_CONNECT:
          Serial.println("Connect error");
          break;
        case DHT20_MISSING_BYTES:
          Serial.println("Missing bytes");
          break;
        case DHT20_ERROR_BYTES_ALL_ZERO:
          Serial.println("All bytes read zero");
          break;
        case DHT20_ERROR_READ_TIMEOUT:
          Serial.println("Read time out");
          break;
        case DHT20_ERROR_LASTREAD:
          Serial.println("Error read too fast");
          break;
        default:
          Serial.println("Unknown error");
          break;
      }
  }
  return data;
}

void buzzer_setup()
{
    // INITIALIZING BUZZER VALUES
    pinMode(BUZZER_PIN, OUTPUT); // Configure buzzer pin
    buzzer_timer = millis() + BUZZER_OFF_TIME;
    buzzer_state = BUZZER_OFF_STATE;
}

void buzz(unsigned long on)
{
    tone(BUZZER_PIN, 10);
    delay(on);
    noTone(BUZZER_PIN);
}

void buzzerSwitch() 
{
    switch(buzzer_state)
    {
      case BUZZER_OFF_STATE:
        if (millis() >= buzzer_timer)
            buzzer_state = BUZZER_ON_STATE;
        break;
      case BUZZER_ON_STATE:
        buzz(BUZZER_ON_TIME);
        buzzer_timer = millis() + BUZZER_OFF_TIME;
        buzzer_state = BUZZER_OFF_STATE;
        break;
    }
}

void display_setup() 
{
  ttg.init();
  ttg.setRotation(1);
  ttg.setTextSize(1);
  ttg.setTextColor(TFT_WHITE);
  ttg.fillScreen(TFT_BLACK);
}

void display_loop(SensorData sensor_val)
{
  if (sensor_val.temp == "0" && sensor_val.moisture == "0" && sensor_val.light == "0")
    return;

  ttg.setTextSize(2);
  ttg.setTextColor(TFT_WHITE);
  ttg.fillScreen(TFT_BLACK);

  ttg.drawString("Temp.: " + sensor_val.temp + " C", 0, 0, 1);
  
  // checking moisture levels
  if (sensor_val.moisture.toFloat() < dry) 
    ttg.drawString("Low Moist.: " + sensor_val.moisture + "%", 0, 32, 1);
  else
    ttg.drawString("Moist.: " + sensor_val.moisture + "%", 0, 32, 1);
  
  int mappedValue = map(sensor_val.light.toFloat(), lightMin, lightMax, desiredMin, desiredMax);

  if (sensor_val.light.toFloat() < shade)
    ttg.drawString("Low Light: " + String(mappedValue) + "%", 0, 64, 1);
  else
    ttg.drawString("Light: " +  String(mappedValue) + "%", 0, 64, 1);
  
  // if (needs_water) 
  // {
  //   ttg.setTextColor(TFT_RED);
  //   ttg.drawString("Watering Countdown: " + days, 0, 96, 1);
  // }

}

void setup() 
{
  pinMode(PHOTORESISTOR_PIN, INPUT);
  Serial.begin(9600);
  delay(1000);

  display_setup();
  buzzer_setup();
  sensor_data_setup();
  aws_setup(); // Uncomment for testing AWS
}

void loop() 
{
  buzzerSwitch(); // switch case between buzzer's on and off states
  SensorData sensor_val = sensor_data_loop();
  //Serial.println("Temperature: " + sensor_val.temp + " Moisture: " + sensor_val.moisture + " Light: " + sensor_val.light); // Uncomment for testing sensor data, comment AWS out

  display_loop(sensor_val);
  aws_loop(aws_loop_msg(sensor_val)); // Uncomment for testing AWS
  delay(1000);
}
