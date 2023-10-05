#include <ArduinoBLE.h>
//#include <EEPROM.h>
#define SEC 1000
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#include <U8g2lib.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <NanoBLEFlashPrefs.h>
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
//NanoBLEFlashPrefs myFlashPrefs;
void TaskHandler(void* pvParameters);
void TaskTemp(void* pvParameters);
TaskHandle_t Task1;  //check temperature in real time should keep the right temp on th eplate
TaskHandle_t Task2;  //check user input so button and BLE and also print to OLED

//TODO definne pins
#define ONE_WIRE_BUS_U 26  // Pin Arduino a cui colleghiamo il pin DQ del sensore
#define ONE_WIRE_BUS_D 27  // Pin Arduino a cui colleghiamo il pin DQ del sensore
#define Rel_U 32           //pin for relay of upper plate
#define Rel_D 33           //pin for relay of upper plate
const int plus_U = 16;     //pin for btn of upper plate
const int minus_U = 17;    //pin for btn of upper plate
const int plus_D = 18;     //pin for btn of lower plate
const int minus_D = 25;    //pin for btn of lower plate

// define the 2 thermistor
OneWire oneWire_U(ONE_WIRE_BUS_U);        // Imposta la connessione OneWire
DallasTemperature sensore_U(&oneWire_U);  // Dichiarazione dell'oggetto sensore
OneWire oneWire_D(ONE_WIRE_BUS_D);        // Imposta la connessione OneWire
DallasTemperature sensore_D(&oneWire_D);  // Dichiarazione dell'oggetto sensore
//BLE config
BLEService TempService("W420");
BLEIntCharacteristic TempUp("W421", BLERead | BLEWrite);
BLEIntCharacteristic TempDown("W422", BLERead | BLEWrite);
BLEIntCharacteristic ReadUp("W423", BLERead | BLENotify);
BLEIntCharacteristic ReadDown("W424", BLERead | BLENotify);
BLEDescriptor TU_desc("W421", "TEMPERATURE SETTED UP - (Read|Write)");
BLEDescriptor TD_desc("W422", "TEMPERATURE SETTED DOWN - (Read|Write)");
BLEDescriptor RU_desc("W423", "TEMPERATURE READED UP - (Read|Notify)");
BLEDescriptor RD_desc("W424", "TEMPERATURE READED DOWN - (Read|Notify)");
int TU = 100;
int TD = 100;
float RU;
float RD;
int p_U;
int p_D;
int m_U;
int m_D;

void u8g2_prepare() {  //prepare the screen
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void setup() {
  //initialize
  Serial.begin(115200);
  u8g2.begin();
  u8g2_prepare();
  pinMode(Rel_D, OUTPUT);
  pinMode(Rel_U, OUTPUT);
  pinMode(plus_D, INPUT_PULLUP);
  pinMode(plus_U, INPUT_PULLUP);
  pinMode(minus_D, INPUT_PULLUP);
  pinMode(minus_U, INPUT_PULLUP);
  //pinMode(LED_BUILTIN, OUTPUT); // initialize the built-in LED pin to indicate when a central is connected
  sensore_U.begin();
  sensore_D.begin();
  sensore_U.requestTemperatures();
  sensore_D.requestTemperatures();
  // BLE begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1)
      ;
  }
  BLE.setLocalName("PRESSING");
  BLE.setDeviceName("PRESSING");
  BLE.setAdvertisedService(TempService);
  TempUp.addDescriptor(TU_desc);
  TempDown.addDescriptor(TD_desc);
  ReadUp.addDescriptor(RU_desc);
  ReadDown.addDescriptor(RD_desc);
  TempService.addCharacteristic(TempUp);
  TempService.addCharacteristic(TempDown);
  TempService.addCharacteristic(ReadUp);
  TempService.addCharacteristic(ReadDown);
  BLE.addService(TempService);
  ReadUp.setValue(sensore_U.getTempCByIndex(0));
  ReadDown.setValue(sensore_D.getTempCByIndex(0));
  setValue();
  BLE.advertise();



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
void loop() {}
void TaskHandler(void* pvParameters) {
  while (1) {
    Serial.println("Task 1");
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
    ReadUp.setValue(RU);
    ReadDown.setValue(RD);
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

    //BLE Check
    if (BLE.central()) {
      //digitalWrite(LED_BUILTIN, HIGH);
      if (TU != TempUp.value()) {
        TU = TempUp.value();
        //myFlashPrefs.writePrefs(&TU, sizeof(TU));
      }
      if (TD != TempDown.value()) {
        TD = TempDown.value();
        //myFlashPrefs.writePrefs(&TD, sizeof(TD));
      }
    }
    //EEPROM.write(0,TU);
    //EEPROM.write(1,TD);

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

void TaskTemp(void* pvParameters) {
  while (1) {
    //Serial.println("Temp Task");
    if (TU > RU) {
      Serial.println("UP HIGH");
      digitalWrite(Rel_U, HIGH);
    } else {
      Serial.println("UP LOW");
      digitalWrite(Rel_U, LOW);
    }
    if (TD > RD) {
      Serial.println("DOWN HIGH");
      digitalWrite(Rel_D, HIGH);
    } else {
      Serial.println("DOWN LOW");
      digitalWrite(Rel_D, LOW);
    }
    vTaskDelay(100);
  }
}
void setValue() {
  Serial.println("SET VALUE");
  //TU = EEPROM.read(0);
  //TD = EEPROM.read(1);
  /*TU = myFlashPrefs.readPrefs(&TU, sizeof(TU));
  TD = myFlashPrefs.readPrefs(&TD, sizeof(TD));*/
  TempUp.setValue(TU);
  TempDown.setValue(TD);
}
