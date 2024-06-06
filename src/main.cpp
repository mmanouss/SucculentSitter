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
#include <queue>
#include <vector>
#include <cmath>
#include <iomanip>

// #include "algorithms.h"

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

double determinant(double **a, const size_t k);
void transpose(double **num, double **fac, double **inverse, const size_t r);
void cofactor(double **num, double **inverse, const size_t f);
void cofactor(double **num, double **inverse, const size_t f);
double **Make2DArray(const size_t rows, const size_t cols);
double **MatTrans(double **array, const size_t rows, const size_t cols);
double **MatMul(const size_t m1, const size_t m2, const size_t m3, double **A, double **B);
void MatVectMul(const size_t m1, const size_t m2, double **A, double *v, double *Av);
void PolyFit(std::queue<unsigned long> x, std::queue<double>y, const size_t n, const size_t k, const bool fixedinter,
const double fixedinterval, double *beta, double **Weights, double **XTWXInv);
double * queue_to_array(std::queue<double> q);
unsigned long * queue_to_array(std::queue<unsigned long> q);
double predictHumidity(std::queue<double> recordedHumidities, std::queue<unsigned long> recordedTimes, unsigned long t, size_t degree);
size_t prediction_data_size = 75;
std::queue<double> humidities;
std::queue<unsigned long> times;
bool needs_water_in_hour = false;

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
    return "Temperature: " + sensor_val.temp + "°C / " + f + "°F, Moisture (LOW): " + sensor_val.moisture + "%, Light (LOW):" + String(mappedValue) + "%";
  else if (sensor_val.moisture.toFloat() < dry)
    return "Temperature: " + sensor_val.temp + "°C / " + f + "°F, Moisture (LOW): " + sensor_val.moisture + "%, Light: " + String(mappedValue) + "%";
  else if (sensor_val.light.toFloat() < shade)
    return "Temperature: " + sensor_val.temp + "°C / " + f + "°F, Moisture: " + sensor_val.moisture + "%, Light (LOW): " + String(mappedValue) + "%";
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
      Serial.print("\n");
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

bool predictWatering(SensorData sensor_vals, unsigned long t) 
{
  double threshold = 50;

  String str = sensor_vals.moisture;
  double current_humidity = strtod(str.begin(), nullptr);
  unsigned long an_hour_from_now = t + 3.6e+6;
  size_t degree = 5;
  // double predicted_humidity = predictHumidity(humidities, times, an_hour_from_now, degree);

  // return (predicted_humidity > threshold);
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
  
  // // predicting next time to water plants
  // if (needs_water_in_hour) {
  //   // if changing the color causes problems, you can get rid of this
  //   // also i don't know if this y value will show up on the screen
  //   ttg.setTextColor(TFT_RED);
  //   ttg.drawString("Warning: May want to water plant in the next hour", 0, 96, 1);
  //   ttg.setTextColor(TFT_WHITE);
  // }

}

// Calculates the determinant of a matrix 
double determinant(double **a, const size_t k) {
    double s = 1;
    double det = 0.;
    double **b = Make2DArray(k,k);
    size_t m;
    size_t n;

    if (k == 1) return (a[0][0]);
    for (size_t c=0; c<k; c++) {
        m = 0;
        n = 0;

        for (size_t i = 0; i < k; i++) {
            for (size_t j = 0; j < k; j++) {
                b[i][j] = 0;
                if (i != 0 && j != c) {
                    b[m][n] = a[i][j];
                    if (n < (k - 2)) {
                        n++;
                    }
                    else {
                        n = 0;
                        m++;
                    }
                }
            }
        }
        det = det + s * (a[0][c] * determinant(b, k - 1));
        s = -1 * s;
    }
    return (det);
}

// Perform the transposition
void transpose(double **num, double **fac, double **inverse, const size_t r) {
    double **b = Make2DArray(r,r);
    double deter;

    for (size_t i=0; i<r; i++) {
        for (size_t j=0; j<r; j++) {
            b[i][j] = fac[j][i];
        }
    }

    deter = determinant(num, r);

    for (size_t i=0; i<r; i++) {
        for (size_t j=0; j<r; j++) {
            inverse[i][j] = b[i][j] / deter;
        }
    }
}

// Calculates the cofactors 
void cofactor(double **num, double **inverse, const size_t f)
{
    double **b = Make2DArray(f,f);
    double **fac = Make2DArray(f,f);
   
    size_t m;
    size_t n;

    for (size_t q=0; q<f; q++) {
        for (size_t p=0; p<f; p++) {
            m = 0;
            n = 0;
            for (size_t i=0; i<f; i++) {
                for (size_t j=0; j<f; j++) {
                    if (i != q && j != p) {
                        b[m][n] = num[i][j];
                        if (n < (f - 2)) {
                            n++;
                        }
                        else {
                            n = 0;
                            m++;
                        }
                    }
                }
            }
            fac[q][p] = pow(-1, q + p) * determinant(b, f - 1);
        }
    }

    transpose(num, fac, inverse, f);
}

// Initialize a 2D array
double **Make2DArray(const size_t rows, const size_t cols) {

    double **array;

    array = new double*[rows];
    for(size_t i = 0; i < rows; i++) {
        array[i] = new double[cols];
    }

    for(size_t i = 0; i < rows; i++) {
        for(size_t j = 0; j < cols; j++) {
            array[i][j] = 0.;
        }
    }
    
    return array;

}

// Transpose a 2D array
double **MatTrans(double **array, const size_t rows, const size_t cols) {

    double **arrayT = Make2DArray(cols,rows);

    for(size_t i = 0; i < rows; i++) {
        for(size_t j = 0; j < cols; j++) {
            arrayT[j][i] = array[i][j];
        }
    }
    
    return arrayT;

}

// Perform the multiplication of matrix A[m1,m2] by B[m2,m3]
double **MatMul(const size_t m1, const size_t m2, const size_t m3, double **A, double **B) {

    double **array = Make2DArray(m1,m3);

    for (size_t i=0; i<m1; i++) {          
        for (size_t j=0; j<m3; j++) {      
            array[i][j]=0.; 
            for (size_t m=0; m<m2; m++) {
                array[i][j]+=A[i][m]*B[m][j];
            } 
        }       
    }
    return array;

}

// Perform the multiplication of matrix A[m1,m2] by vector v[m2,1]
void MatVectMul(const size_t m1, const size_t m2, double **A, double *v, double *Av) {

    
    for (size_t i=0; i<m1; i++) {   
        Av[i]=0.;
        for (size_t j=0; j<m2; j++) {
            Av[i]+=A[i][j]*v[j];    
        } 
    }
   
}

void PolyFit(std::queue<unsigned long> xq, std::queue<double>yq, const size_t n, const size_t k, const bool fixedinter,
const double fixedinterval, double *beta, double **Weights, double **XTWXInv){ 
  
    // Definition of variables

    unsigned long * x = queue_to_array(xq);
    double * y = queue_to_array(yq);

    double **X = Make2DArray(n,k+1);           // [n,k+1]
    double **XT;                               // [k+1,n]
    double **XTW;                              // [k+1,n]
    double **XTWX;                             // [k+1,k+1]

    double *XTWY = new double[k+1];
    double *Y = new double[n];

    size_t begin = 0;
    if (fixedinter) begin = 1;

    // Initialize X
    for (size_t i=0; i<n; i++) { 
        for (size_t j=begin; j<(k+1); j++) {  // begin
          X[i][j]=pow(x[i],j);  
        }       
    } 

    // Matrix calculations
    XT = MatTrans(X, n, k+1);                 // Calculate XT
    XTW = MatMul(k+1,n,n,XT,Weights);         // Calculate XT*W
    XTWX = MatMul(k+1,n,k+1,XTW,X);           // Calculate (XTW)*X

    if (fixedinter) XTWX[0][0] = 1.;  
    
    cofactor(XTWX, XTWXInv, k+1);             // Calculate (XTWX)^-1

    for (size_t m=0; m<n; m++) {
        if (fixedinter) {
            Y[m]= y[m]-fixedinterval;
        } 
        else {
            Y[m] = y[m];
        }
    } 
    MatVectMul(k+1,n,XTW,Y,XTWY);             // Calculate (XTW)*Y
    MatVectMul(k+1,k+1,XTWXInv,XTWY,beta);    // Calculate beta = (XTWXInv)*XTWY

    if (fixedinter) beta[0] = fixedinterval;

}

double * queue_to_array(std::queue<double> q) {
    size_t q_size = q.size();
    double *array = new double [q_size];
    for (int i=0; i < q_size; i++){
        array[i] = q.front();
        q.pop();
    }
    return array;
}

unsigned long * queue_to_array(std::queue<unsigned long> q) {
    size_t q_size = q.size();
    unsigned long *array = new unsigned long [q_size];
    for (int i=0; i < q_size; i++){
        array[i] = q.front();
        q.pop();
    }
    return array;
}

double predictHumidity(std::queue<double> recordedHumidities, std::queue<unsigned long> recordedTimes, unsigned long t, size_t degree) {
    // takes last N recorded times produces a polynomial regression 
    double hypothesizedHumidity = 0.0;
    unsigned long millis();

    std::vector<double> coefficients;
    
    size_t n = recordedTimes.size(); // length of recordedTimes
    size_t k = degree;

    bool fixedinter = false; 
    double fixedinterval = 0.;                       // The fixed intercept value (if applicable)
    double coefbeta[k+1];                            // Coefficients of the polynomial
    double **XTWXInv = Make2DArray(k+1,k+1);
    double **Weights = Make2DArray(n,n);

    PolyFit(recordedTimes, recordedHumidities, n, k, fixedinter, fixedinterval, coefbeta, Weights, XTWXInv);

    int deg = coefficients.size(); // this is really degree-1
    for (int i=0; i < (int)deg + 1; i++){
        hypothesizedHumidity += coefficients[i] * (pow((double)t, i)); // calculate results of plugging in t value to poly function
    }

    recordedHumidities.pop(); // get rid of earliest humidity entry
    recordedHumidities.push(hypothesizedHumidity); // add latest humidity entry
    recordedTimes.pop(); // get rid of earliest time entry
    recordedTimes.push(millis()); // add latest time entry

    return hypothesizedHumidity;
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
  unsigned long t = millis();
  Serial.println("Temperature: " + sensor_val.temp + " Moisture: " + sensor_val.moisture + " Light: " + sensor_val.light); // Uncomment for testing sensor data, comment AWS out
  
  if (humidities.size() >= prediction_data_size){
    // if we have enough data to do the prediction then do it
    needs_water_in_hour = predictWatering(sensor_val, t);
  } else {
    humidities.push(strtod(sensor_val.moisture.begin(), nullptr));
    times.push(t);
  }

  display_loop(sensor_val);
  aws_loop(aws_loop_msg(sensor_val)); // Uncomment for testing AWS
  delay(1000);
}
