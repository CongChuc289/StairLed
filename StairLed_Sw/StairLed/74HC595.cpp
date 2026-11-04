#include "74HC595.h"
#include <EEPROM.h>

ConfigData config;
uint32_t current_led_mask = 0;

static uint32_t last_hw_mask = 0xFFFFFFFF;
static bool hw_initialized = false;

void shift595_init(void) {
  pinMode(PIN_LAT, OUTPUT);
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);

  digitalWrite(PIN_LAT, LOW);
  digitalWrite(PIN_CLK, LOW);
  digitalWrite(PIN_DATA, LOW);

  // Khởi tạo ban đầu: tắt toàn bộ 32 LED, bắt buộc ghi ra phần cứng
  hw_initialized = false;
  current_led_mask = 0;
  shift595_write32(0, true);
}

// ====================================================================
// XUẤT 32 BIT RA 4 IC 74HC595 MẮC NỐI TIẾP (DAISY-CHAIN)
// Phân chia 4 IC điều khiển 32 bậc LED:
//   - IC1 (U1): Bậc 1  đến Bậc 8  (Bit 0  .. Bit 7 , ngõ ra O0_HC  .. O7_HC )
//   - IC2 (U2): Bậc 9  đến Bậc 16 (Bit 8  .. Bit 15, ngõ ra O8_HC  .. O15_HC)
//   - IC3 (U3): Bậc 17 đến Bậc 24 (Bit 16 .. Bit 23, ngõ ra O16_HC .. O23_HC)
//   - IC4 (U4): Bậc 25 đến Bậc 32 (Bit 24 .. Bit 31, ngõ ra O24_HC .. O31_HC)
//
// Cố định phần cứng: Mức 1 (HIGH) mở MOSFET dẫn sáng LED
// Có timing delayMicroseconds chuẩn đảm bảo xung vuông 250kHz, chống trượt xung
// Tối ưu hóa: Bỏ qua ghi nếu trạng thái mask không thay đổi -> Giảm 100% CPU lock
// ====================================================================
void shift595_write32(uint32_t mask, bool force) {
  current_led_mask = mask;

  // Nếu dữ liệu không đổi và không bắt buộc cập nhật, thoát ngay lập tức
  if (hw_initialized && !force && mask == last_hw_mask) {
    return;
  }
  last_hw_mask = mask;
  hw_initialized = true;

  digitalWrite(PIN_LAT, LOW);
  delayMicroseconds(2);

  // Dịch 32 bit từ MSB (bit 31) đến LSB (bit 0):
  for (int8_t b = 31; b >= 0; b--) {
    bool bitVal = (mask >> b) & 0x01;
    digitalWrite(PIN_DATA, bitVal ? HIGH : LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_CLK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_CLK, LOW);
    delayMicroseconds(2);
  }

  // Chốt dữ liệu ra output của cả 4 IC 74HC595 đồng thời
  digitalWrite(PIN_LAT, HIGH);
  delayMicroseconds(5);
  digitalWrite(PIN_LAT, LOW);
  delayMicroseconds(2);
}

void shift595_set_mask(uint32_t mask) {
  shift595_write32(mask);
}

void shift595_set_pin(uint8_t pin, bool state) {
  if (pin >= MAX_STEPS) return;
  if (state) {
    current_led_mask |= ((uint32_t)1 << pin);
  } else {
    current_led_mask &= ~((uint32_t)1 << pin);
  }
  shift595_write32(current_led_mask);
}

void shift595_set_all(bool state) {
  uint32_t mask = 0;
  if (state) {
    if (config.number_step >= MAX_STEPS) {
      mask = 0xFFFFFFFF;
    } else {
      mask = ((uint32_t)1 << config.number_step) - 1;
    }
  }
  shift595_write32(mask);
}

uint32_t shift595_get_mask(void) {
  return current_led_mask;
}

// Quản lý EEPROM
void eeprom_reset_defaults(void) {
  config.magic = EEPROM_MAGIC;
  config.number_step = 25;       // Mặc định 25 bậc (hỗ trợ điều chỉnh 1-32)
  config.t0 = 150;               // t0: Thời gian sáng từng bậc (ms)
  config.t1 = 150;               // t1: Thời gian tắt từng bậc (ms)
  config.t2 = 5000;              // t2: Thời gian sáng sau khi rời cầu thang (ms)
  config.mode = MODE_SMART_SENSOR; // Chế độ cảm biến thông minh

  EEPROM.put(0, config);
  EEPROM.commit();
  Serial.println(F("[EEPROM] Đã khôi phục cấu hình mặc định (t0=150ms, t1=150ms, t2=5000ms)!"));
}

void eeprom_init_and_load(void) {
  EEPROM.begin(sizeof(ConfigData) + 8);
  EEPROM.get(0, config);

  if (config.magic != EEPROM_MAGIC || config.number_step == 0 || config.number_step > MAX_STEPS) {
    Serial.println(F("[EEPROM] Dữ liệu chưa khởi tạo hoặc không hợp lệ. Đang reset defaults..."));
    eeprom_reset_defaults();
  } else {
    bool need_re_save = false;
    if (config.number_step < 1 || config.number_step > MAX_STEPS) {
      config.number_step = 25;
      need_re_save = true;
    }
    if (config.t0 < 20 || config.t0 > 5000) {
      config.t0 = 150;
      need_re_save = true;
    }
    if (config.t1 < 20 || config.t1 > 5000) {
      config.t1 = 150;
      need_re_save = true;
    }
    if (config.t2 < 500 || config.t2 > 60000) {
      config.t2 = 5000;
      need_re_save = true;
    }
    if (need_re_save) {
      eeprom_save();
    }
    Serial.println(F("[EEPROM] Nạp cấu hình thành công:"));
    Serial.printf_P(PSTR(" - Số bậc (N): %u\n"), config.number_step);
    Serial.printf_P(PSTR(" - Thời gian sáng từng bậc (t0): %u ms\n"), config.t0);
    Serial.printf_P(PSTR(" - Thời gian tắt từng bậc (t1): %u ms\n"), config.t1);
    Serial.printf_P(PSTR(" - Thời gian giữ sáng sau khi rời cầu thang (t2): %u ms\n"), config.t2);
    Serial.printf_P(PSTR(" - Chế độ (Mode): %u\n"), config.mode);
  }
}

void eeprom_save(void) {
  ConfigData stored;
  EEPROM.get(0, stored);
  config.magic = EEPROM_MAGIC;

  // So sánh dữ liệu trước khi commit vào Flash -> Tránh hao mòn Flash sector khi chạy 24/7
  if (memcmp(&stored, &config, sizeof(ConfigData)) != 0) {
    EEPROM.put(0, config);
    EEPROM.commit();
    Serial.println(F("[EEPROM] Đã lưu cấu hình mới vào Flash thành công!"));
  } else {
    Serial.println(F("[EEPROM] Cấu hình không thay đổi, bỏ qua ghi Flash."));
  }
}
