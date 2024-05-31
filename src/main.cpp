#include <Arduino.h>
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
#define PHOTORESISTOR_PIN 13

// DEFINING BUZZER VALUES
#define BUZZER_OFF_STATE 0
#define BUZZER_ON_STATE 1
#define BUZZER_OFF_TIME 1800000 // 30min * 60s/min = 1800s * 1000ms/s = 1800000ms
#define BUZZER_ON_TIME 10000 // 10s * 1000ms/s = 10000ms
int buzzer_state; // Buzzer state
unsigned long buzzer_timer; // Buzzer timer

TFT_eSPI ttg = TFT_eSPI(); 
void p1_oled_display(String message);

// Server details
const char serverAddress[] = "3.144.71.254"; // adjust with instance
const int serverPort = 5000;

// This example downloads the URL "http://arduino.cc/"
char ssid[50]; // your network SSID (name)
char pass[50]; // your network password (use for WPA, or use as key for WEP)

const char kHostname[] = "worldtimeapi.org"; // Name of the server we want to connect to
const char kPath[] = "/api/timezone/Europe/London.txt"; // Path to download (this is the bit after the hostname in the URL that you want to download

const int kNetworkTimeout = 30 * 1000; // num of ms to wait without receiving any data before we give up
const int kNetworkDelay = 1000; // num of ms to wait if no data is available before trying again

// DHT initialization
DHT20 DHT;
uint8_t count_var = 0;

// Photoresistor initialization
int max_light = 0; // initialize maximum light sensor value
int shade = 2500; // this number and below indicates not in a sunny spot
int mapped_value; // mapped min and max light values

// Function declarations
void nvs_access();
void aws_setup();
void aws_loop(const String & send_val);

String temp_humidity_light();
void sensor_data_setup();
String sensor_data_loop();

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
    delay(1000);
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

void aws_loop(const String & send_val)
{
    int err = 0;
    WiFiClient c;
    HttpClient http(c);
    //err = http.get(kHostname, kPath); // UNCOMMENT WHEN TESTING, COMMENT BELOW LINE

    String path = "/?var=" + send_val;
    err = http.get(serverAddress, serverPort, path.c_str(), NULL);
    if (err == 0) 
    {
        Serial.println("startedRequest ok");
        err = http.responseStatusCode();
        if (err >= 0) {
            Serial.print("Got status code: ");
            Serial.println(err);
            // Usually you'd check that the response code is 200 or a similar "success" code (200-299) before carrying on,
            // but we'll print out whatever response we get
            err = http.skipResponseHeaders();
            if (err >= 0) 
            {
                int bodyLen = http.contentLength();
                Serial.print("Content length is: ");
                Serial.println(bodyLen);
                Serial.println();
                Serial.println("Body returned follows:");
                // Now we've got to the body, so we can print it out
                unsigned long timeoutStart = millis();
                char c;
                // Whilst we haven't timed out & haven't reached the end of the body
                while ((http.connected() || http.available()) && ((millis() - timeoutStart) < kNetworkTimeout)) {
                    if (http.available()) {
                        c = http.read();
                        // Print out this character
                        Serial.print(c);
                        bodyLen--;
                        // We read something, reset the timeout counter
                        timeoutStart = millis();
                    } else {
                        // We haven't got any data, so let's pause to allow some to arrive
                        delay(kNetworkDelay);
                    }
                }
            } 
            else 
            {
                Serial.print("Failed to skip response headers: ");
                Serial.println(err);
            }
        } 
        else 
        {
            Serial.print("Getting response failed: ");
            Serial.println(err);
        }
    } 
    else 
    {
        Serial.print("Connect failed: ");
        Serial.println(err);
    }
    http.stop();
    
    // And just stop, now that we've tried a download
    // while (1);
    delay(2000); // COMMENT OUT WHEN TESTING
}

String temp_humidity_light() 
{
    float t = DHT.getTemperature();
    float h = DHT.getHumidity();
    int l = analogRead(PHOTORESISTOR_PIN);

    String temp = String(t, 2); // 2 decimals
    String humid = String(h, 2); // 2 decimals
    String light = String(l);

    // for AWS: "Temperature%20=%20" + temp + ",%20Humidity%20=%20" + humid;
    return "Temperature: " + temp + " Humidity: " + humid + " Light: " + light;
}

void sensor_data_setup()
{
  Wire.begin();
  DHT.begin(); // ESP32 default pins 21 22

  delay(1000);
}

String sensor_data_loop()
{
    if (millis() - DHT.lastRead() >= 1000)
    {
      //  READ DATA
      uint32_t start = micros();
      int status = DHT.read();
      uint32_t stop = micros();

      if ((count_var % 10) == 0)
      {
        count_var = 0;
        // Serial.println("\nType\tHumidity (%)\tTemp (°C)\tTime (µs)\tStatus");
      }
      count_var++;

      switch (status)
      {
        case DHT20_OK:
          // Serial.print("OK");
          return temp_humidity_light();
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
      Serial.print("\n");
  }
  return "";
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
            if (millis() >= buzzer_timer){
                buzzer_state = BUZZER_ON_STATE;
            }
        case BUZZER_ON_STATE:
            buzz(BUZZER_ON_TIME);
            buzzer_timer = millis() + BUZZER_OFF_TIME;
            buzzer_state = BUZZER_OFF_STATE;
    }
}

void display_setup() 
{
  ttg.init();
  ttg.setRotation(1);
}

void p1_oled_display(String message)
{
  if (message == "None")
    return;

    ttg.println();
    ttg.setTextDatum(MC_DATUM);
    ttg.setTextColor(TFT_WHITE);
    ttg.fillScreen(TFT_BLACK);
    ttg.drawString(message, 120, 60, 6);
  
}

void display_loop(String message)
{
  p1_oled_display(message);
}

void setup() 
{
  pinMode(PHOTORESISTOR_PIN, INPUT);
  Serial.begin(9600);

  display_setup();
  buzzer_setup();
  sensor_data_setup();
  //aws_setup(); // Comment out for testing functionality
}

void loop() 
{
  buzzerSwitch(); // switch case between buzzer's on and off states

  String send_val = sensor_data_loop();
  if (send_val != "")
  {
    Serial.println(send_val);
    display_loop(send_val);
    //aws_loop(send_val); // Comment out for testing functionality
  }
}




