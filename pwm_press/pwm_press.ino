/**
    press up use channel 1
    press down use channel 2
    220V Home is at 50Hz
    resolution = 8
*/
//ALL Elegant OTA Part
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>

const char *ssid = "PRESS";
const char *password = "PR355_P455W0RD";

WebServer server(80);

unsigned long ota_progress_millis = 0;

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}
//-----------------------------------------------------------

#define SEC 1000
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#include <U8g2lib.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>


//#include <NanoBLEFlashPrefs.h>
#include "hal/ledc_types.h"
#include <Arduino.h>
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void TaskHandler(void* pvParameters);
void TaskTemp(void* pvParameters);
TaskHandle_t Task1;  //check temperature in real time should keep the right temp on th eplate
TaskHandle_t Task2;  //check user input so button and BLE and also print to OLED

// define pins
#define ONE_WIRE_BUS_U 26  // Pin Arduino a cui colleghiamo il pin DQ del sensore
#define ONE_WIRE_BUS_D 27  // Pin Arduino a cui colleghiamo il pin DQ del sensore
//button for temp
const int plus_U = 16;     //pin for btn of upper plate
const int minus_U = 17;    //pin for btn of upper plate
const int plus_D = 18;     //pin for btn of lower plate
const int minus_D = 19;    //pin for btn of lower plate

const int up_pwm = 22;  // pin for upper pwm
const int down_pwm = 23;  // pin for lower pwm

// define the 2 thermistor
OneWire oneWire_U(ONE_WIRE_BUS_U);        // Imposta la connessione OneWire
DallasTemperature sensore_U(&oneWire_U);  // Dichiarazione dell'oggetto sensore
OneWire oneWire_D(ONE_WIRE_BUS_D);        // Imposta la connessione OneWire
DallasTemperature sensore_D(&oneWire_D);  // Dichiarazione dell'oggetto sensore

int TU = 100;   //Set point for upper plate
int TD = 100; //Set point for lower plate
float RU; //Read temp value upper plate
float RD; //Read temp value lower plate
int p_U;  //Button status plus up
int p_D;  //Button status minus up
int m_U;  //Button status plus down
int m_D;  //Button status minus down
int pwm_UP = 0;
int pwm_DOWN = 0;
// setting PWM properties
const int freq = 50;
const int up_pwm_channel = 0;  //channel for upper plate, use differnt channel because goes to different freq related to read temp
const int down_pwm_channel = 1;
const int resolution = 8;
const int MAX_DUTY_CYCLE = (int)(pow(2, resolution) - 1);

void u8g2_prepare() {  //prepare the screen
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void setup() {
  Serial.begin(115200);
IPAddress myIP = WiFi.softAP(ssid, password);
  if (!myIP) {
    log_e("Soft AP creation failed.");
    while (1);
  }else{
    Serial.println(myIP);
  }
  server.begin();
   server.on("/", []() {
    server.send(200, "text/plain", "Hi! This is ElegantOTA Demo.");
  });

  ElegantOTA.begin(&server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();


  //begin screen
  u8g2.begin();
  u8g2_prepare();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
  pinMode(plus_D, INPUT_PULLUP);
  pinMode(plus_U, INPUT_PULLUP);
  pinMode(minus_D, INPUT_PULLUP);
  pinMode(minus_U, INPUT_PULLUP);
  //create pwm channel and set to pin
  /*ledcSetup(up_pwm_channel, freq, resolution);
  ledcAttachPin(up_pwm, up_pwm_channel);
  ledcWrite(up_pwm, 255);*/
  bool p_u = ledcAttachChannel(up_pwm, freq, resolution,up_pwm_channel);
  bool p_d = ledcAttachChannel(down_pwm, freq, resolution,down_pwm_channel);
  Serial.print("PWM1 attach channel is ");
  if (p_u && p_d){
    Serial.println("PWM CREATED ");
    ledcWrite(up_pwm,255);
    ledcWrite(down_pwm,255);
  }else{
    Serial.println("ERROR CREATING PWM");
  }
  //pinMode(up_pwm, OUTPUT);
  
  /*ledcSetup(down_pwm_channel, freq, resolution);  
  ledcAttachPin(down_pwm, down_pwm_channel);  
  ledcAttach(down_pwm, freq, resolution);
  
  analogWrite(up_pwm, 255);
  pinMode(down_pwm, OUTPUT);*/
  
  
  //pinMode(LED_BUILTIN, OUTPUT); // initialize the built-in LED pin to indicate when a central is connected
  //begin temp sensor
  sensore_U.begin();
  sensore_D.begin();
  sensore_U.requestTemperatures();
  sensore_D.requestTemperatures();


  //define tasks
  xTaskCreatePinnedToCore(
    TaskTemp, /* Task function. */
    "Task1",  /* name of task. */
    10000,    /* Stack size of task */
    NULL,     /* parameter of the task */
    1,        /* priority of the task */
    &Task1,   /* Task handle to keep track of created task */
    0);       /* pin task to core 0 */

  xTaskCreatePinnedToCore(
    TaskHandler, /* Task function. */
    "Task2",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task2,      /* Task handle to keep track of created task */
    1);
}

void loop() {
  // put your main code here, to run repeatedly:

}

void TaskHandler(void* pvParameters) {
  while (1) {
    server.handleClient();
    ElegantOTA.loop();

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setFontRefHeightExtendedText();
    u8g2.setDrawColor(1);
    u8g2.setFontPosTop();
    u8g2.setFontDirection(0);
    //temp sensor update
    sensore_U.requestTemperatures();    
    sensore_D.requestTemperatures();
    RU = sensore_U.getTempCByIndex(0);
    RD = sensore_D.getTempCByIndex(0);
    
    p_U = digitalRead(plus_U);
    if (p_U == HIGH) {
      TU = TU + 1;
    }
    p_D = digitalRead(plus_D);
    if (p_D == HIGH) {
      TD = TD + 1;
    }
    m_U = digitalRead(minus_U);
    if (m_U == HIGH) {
      TU = TU - 1;
    }
    m_D = digitalRead(minus_D);
    if (m_D == HIGH) {
      TD = TD - 1;
    }
    u8g2.drawStr(10, 0, "DOWN");
    u8g2.setCursor(70, 0);
    u8g2.print("UP");
    u8g2.setCursor(10, 20);
    u8g2.print(String(TD));
    u8g2.setCursor(70, 20);
    u8g2.print(String(TU));
    u8g2.setCursor(10, 40);
    u8g2.print(String(RD));
    u8g2.setCursor(70, 40);
    u8g2.print(String(RU));
    u8g2.sendBuffer();
    
    vTaskDelay(100);
  }
}
float cutout = 0.70;
void TaskTemp(void* pvParameters) {
  while (1) {
    Serial.print("Task2");
    Serial.println(RU);
    if (RU < (int)(cutout * TU)) {
      bool x = ledcWrite(up_pwm, 255);
    } else if (RU > (int)(cutout * TU) && RU < TU -10){
      pwm_UP = (int)map(RU,cutout *TU, TU-10, 255, 20  );
      ledcWrite(up_pwm, pwm_UP);
    }else{
      ledcWrite(up_pwm, 20);
    }
    
    if (RD < (int)(cutout * TD)) {
      ledcWrite(down_pwm, 255);
    } else if (RD > (int)(cutout * TD) && RD < TD -10){
      pwm_DOWN = (int)map(RD, cutout *TD, TD-10, 255, 20  );
      ledcWrite(down_pwm, pwm_DOWN);
    }else{
      ledcWrite(up_pwm, 20);
    }


    vTaskDelay(100);
  }
}
