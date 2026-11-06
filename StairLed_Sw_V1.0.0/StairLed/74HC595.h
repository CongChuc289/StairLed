#ifndef _74HC595_H
#define _74HC595_H

#include <Arduino.h>
#include "stdint.h"

#define LAT_ON      digitalWrite(D3, HIGH)
#define LAT_OFF     digitalWrite(D3, LOW)
#define CLK_ON      digitalWrite(D4, HIGH)
#define CLK_OFF     digitalWrite(D4, LOW)
#define DATA_ON      digitalWrite(D7, HIGH)
#define DATA_OFF     digitalWrite(D7, LOW)

#define SIZE_USERNAME_LOGIN 30
#define SIZE_PASSWORD_LOGIN 30

typedef enum{
  ST_RESET,
  ST_SET
} STATE;

typedef enum {
  MODE_BASIC = 1,
  MODE_TEST,
  MODE_BLINK,
  MODE_SANG_NHAP_NHAY,
  MODE_SANG_DAN_DON_LEN,
  MODE_SANG_DAN_DON_XUONG,
  MODE_SANG_TONG_HOP,
} MODE;

enum{
  MODE_STANDAR,
  MODE_UP,
  MODE_DOWN,
  MODE_UP_DOWN,
};

typedef struct {
  int timeset;
  int timeoff;
  unsigned char mode; 
  signed char number_step; 
  char username_login[SIZE_USERNAME_LOGIN];
  char password_login[SIZE_PASSWORD_LOGIN];
} buffer_eeprom;

extern buffer_eeprom buffer_config;

void ShiftByte(uint32_t byte_t);
void Scan(uint32_t time_set,uint32_t time_off);
void SetPin(unsigned char Pin, STATE state);
void TogglePin(unsigned char Pin);

#endif
