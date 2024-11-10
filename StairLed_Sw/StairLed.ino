
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "74HC595.h"
#include "webpage.h"
#include "Login.h"

#define BT1  D1
#define BT2  D2


#define SS1 D5
#define SS2 D6
#define ADC A0

extern buffer_eeprom buffer_config;

const char* ssid_ap = "LED_STAIR_0001";
const char* password_ap = "12345678";

uint8_t Dir1 = 0;
uint8_t Dir2 = 0;

uint32_t data = 0;

unsigned int ADC_Value = 0;
unsigned int ADC_Map = 0;

unsigned long last = millis();

extern buffer_eeprom buffer_config;

String DataJson = "";


ICACHE_RAM_ATTR void EXTI1(void) {
  Dir1 = 1;
}
ICACHE_RAM_ATTR void EXTI2(void) {
  Dir2 = 1;
}

ESP8266WebServer    server(80);

void setup_eeprom(void) {
  EEPROM.begin(sizeof(buffer_eeprom));
  EEPROM.get(0, buffer_config);
  Serial.println("Mode: " + String(buffer_config.mode) + " Number_Step: " + String(buffer_config.number_step) + " Time: " + String(buffer_config.timeset) );
}
void mainpage() {
  String s = webpage;
  server.send(200,   "text/html", s );
}

void loginpage() {
  String s = weblogin;
  server.send(200,   "text/html", s );
}

void handleConfig() {
  if (server.method() == HTTP_POST) {
    buffer_config.mode = atoi(server.arg("mode_act").c_str());
    buffer_config.number_step = atoi(server.arg("number_step").c_str());
    buffer_config.timeset = atoi(server.arg("timeset").c_str());
    buffer_config.timeoff = atoi(server.arg("timeoff").c_str());
    if ( (buffer_config.mode <= 0) || (buffer_config.mode > 7) || (buffer_config.number_step > 27)) {
      buffer_config.mode = 2;
      buffer_config.number_step = 27;
    }
    if (buffer_config.number_step <= 0  || (buffer_config.number_step > 27)) {
      buffer_config.number_step = 27;
    }
    EEPROM.put(0, buffer_config);// EEPROM.put
    EEPROM.commit();
    Serial.println("Writed to EEPROM! - Please restart ESP");

    String s = webpage;
    server.send(200,   "text/html", s );
    delay(2000);
    ESP.restart();
  }
}

void handleLogin(void) {
  String s = webpage;
  server.send(200,   "text/html", s );
  server.on("/", mainpage);
}

void sendweb()
{
  DataJson = "";
  DataJson = "{\"mode_act\":\""      + String(buffer_config.mode) + "\"," +
             "\"number_step\":\""       + String(buffer_config.number_step) + "\"," +
             "\"timeset\":\""       + String(buffer_config.timeset) + "\"," +
             "\"timeoff\":\""       + String(buffer_config.timeoff) + "\"," +
             "\"username_login\":\""  + String(buffer_config.username_login) + "\"," +
             "\"password_login\":\""  + String(buffer_config.password_login) + "\"}";

  server.send(200, "text/html", String(DataJson));
}

void ActiveHandle() {
  server.on("/", loginpage);
  server.on("/Login", handleLogin);
  server.on("/Config", handleConfig);
  server.on("/sendweb", sendweb);
  server.begin();
}


void setup() {
  Serial.begin(9600);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(SS1, INPUT_PULLUP);
  pinMode(SS2, INPUT_PULLUP);
  pinMode(BT1, INPUT_PULLUP);
  pinMode(BT2, INPUT_PULLUP);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid_ap , password_ap );
  //WiFi.softAPConfig(ip_ap, gateway_ap, subnet_ap);
  Serial.println("Soft Access Point mode!");
  Serial.println("Please connect WiFi ID: " + String(ssid_ap) + ", Pass: " + String(password_ap)) ;
  Serial.print("Address IP esp: ");
  Serial.println(WiFi.softAPIP());
  setup_eeprom();
  ActiveHandle();
  ShiftByte(0);

  attachInterrupt(digitalPinToInterrupt(SS1), EXTI1, FALLING);
  attachInterrupt(digitalPinToInterrupt(SS2), EXTI2, FALLING);

}
void Func_Blink() {
  for (signed char i = 0; i < buffer_config.number_step; i++) {
    TogglePin(i);
  }
  delay(buffer_config.timeset);
}

void Func_NhapNhay(signed char number) {
  for (signed char a = 0; a < number; a++) {
    for (signed char i = 0; i < buffer_config.number_step; i = i + 2) {
      SetPin(i, ST_SET);
    }
    for (signed char i = 1; i < buffer_config.number_step; i = i + 2) {
      SetPin(i, ST_RESET);
    }
    delay(200);
    for (signed char i = 1; i < buffer_config.number_step; i = i + 2) {
      SetPin(i, ST_SET);
    }
    for (signed char i = 0; i < buffer_config.number_step; i = i + 2) {
      SetPin(i, ST_RESET);
    }
    delay(200);
  }
}

void Func_SangDonLen() {
  for (signed char i = 0; i < buffer_config.number_step; i++) {
    for (signed char j = 0; j < buffer_config.number_step - i; j++) {
      if (millis() - last >= 10) {
        server.handleClient();
        last = millis();
      }
      SetPin(j, ST_SET);
      delay(buffer_config.timeset);
      SetPin(j, ST_RESET);
    }
    SetPin(buffer_config.number_step - 1 - i, ST_SET);
  }
  for (signed char i = 0; i < buffer_config.number_step; i++) {
    if (millis() - last >= 10) {
      server.handleClient();
      last = millis();
    }
    SetPin(i, ST_RESET);
    delay(buffer_config.timeset);
  }
}
void Func_SangDonXuong() {
  for (signed char i = 0; i < buffer_config.number_step; i++) {
    for (signed char j = buffer_config.number_step - 1; j >= i; j--) {
      if (millis() - last >= 10) {
        server.handleClient();
        last = millis();
      }
      SetPin(j, ST_SET);
      delay(buffer_config.timeset);
      SetPin(j, ST_RESET);
    }
    SetPin(i, ST_SET);
  }
  for (signed char i = 0; i < buffer_config.number_step; i++) {
    if (millis() - last >= 10) {
      server.handleClient();
      last = millis();
    }
    SetPin(i, ST_RESET);
    delay(buffer_config.timeset);
  }
}
void loop() {
  if (millis() - last >= 10) {
    server.handleClient();
    last = millis();
  }
  if (buffer_config.mode == MODE_BASIC) {
    Scan(buffer_config.timeset , buffer_config.timeoff);
  }
  else if (buffer_config.mode == MODE_TEST) {
    if (buffer_config.number_step <= 8) data = 0xFF;
    else if (buffer_config.number_step > 8 && buffer_config.number_step <= 16) data = 0xFFFF;
    else if (buffer_config.number_step > 16 && buffer_config.number_step <= 24) data = 0xFFFFFF;
    else if (buffer_config.number_step > 24 && buffer_config.number_step <= 32) data = 0x01FFFFFF;
    ShiftByte(data);
  }
  else if (buffer_config.mode == MODE_BLINK) {
    Func_Blink();
  }
  else if (buffer_config.mode == MODE_SANG_NHAP_NHAY) {
    Func_NhapNhay(1);
  }
  else if (buffer_config.mode == MODE_SANG_DAN_DON_LEN) {
    Func_SangDonLen();
  }
  else if (buffer_config.mode == MODE_SANG_DAN_DON_XUONG) {
    Func_SangDonXuong();
  }
  else if (buffer_config.mode == MODE_SANG_TONG_HOP) {
    Func_SangDonLen();
    Func_NhapNhay(15);
    for (signed char i = 0; i < buffer_config.number_step; i++) {
      if (millis() - last >= 10) {
        server.handleClient();
        last = millis();
      }
      SetPin(i, ST_RESET);
      delay(buffer_config.timeset);
    }
    Func_SangDonXuong();
    Func_NhapNhay(15);
    for (signed char i = 0; i < buffer_config.number_step; i++) {
      if (millis() - last >= 10) {
        server.handleClient();
        last = millis();
      }
      SetPin(i, ST_RESET);
      delay(buffer_config.timeset);
    }
  }
}
