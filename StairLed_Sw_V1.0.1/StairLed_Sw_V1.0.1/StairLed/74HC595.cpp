#include "74HC595.h"
#include <EEPROM.h>

ConfigData config;
uint8_t led_buffer[BUFFER_SIZE] = {0};

static uint8_t last_hw_buffer[BUFFER_SIZE] = {0xFF};
static bool hw_initialized = false;

void shift595_init(void) {
  pinMode(PIN_LAT, OUTPUT);
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);

  digitalWrite(PIN_LAT, LOW);
  digitalWrite(PIN_CLK, LOW);
  digitalWrite(PIN_DATA, LOW);

  // Khởi tạo ban đầu: tắt toàn bộ các bậc, bắt buộc ghi ra phần cứng
  hw_initialized = false;
  memset(led_buffer, 0, sizeof(led_buffer));
  memset(last_hw_buffer, 0xFF, sizeof(last_hw_buffer));
  shift595_update(true);
}

// ====================================================================
// XUẤT BUFFER RA CÁC IC 74HC595 MẮC NỐI TIẾP (DAISY-CHAIN)
// Hỗ trợ số bậc tùy ý (tối đa MAX_STEPS bậc):
//   - IC1: Bậc 1..8   (Bit 0..7  của Byte 0)
//   - IC2: Bậc 9..16  (Bit 0..7  của Byte 1)
//   - IC3: Bậc 17..24 (Bit 0..7  của Byte 2)
//   - IC4: Bậc 25..32 (Bit 0..7  của Byte 3)
//   - IC mở rộng 5..N: Bậc 33..MAX_STEPS (Byte 4..BUFFER_SIZE-1)
//
// Cơ chế Daisy-Chain: IC cuối cùng trong chuỗi sẽ nhận các bit được dịch vào ĐẦU TIÊN.
// Do đó ta dịch từ Byte cao nhất xuống Byte 0, trong mỗi byte dịch từ Bit 7 xuống Bit 0.
// Cố định phần cứng: Mức 1 (HIGH) mở MOSFET dẫn sáng LED
// Timing delayMicroseconds(2) chuẩn xung vuông 250kHz, chống trượt xung khi nối tầng dài
// Tối ưu hóa: Bỏ qua ghi nếu trạng thái buffer không thay đổi -> Giảm 100% CPU lock
// ====================================================================
void shift595_update(bool force) {
  // Nếu dữ liệu không đổi và không bắt buộc cập nhật, thoát ngay lập tức
  if (hw_initialized && !force && memcmp(led_buffer, last_hw_buffer, BUFFER_SIZE) == 0) {
    return;
  }
  memcpy(last_hw_buffer, led_buffer, BUFFER_SIZE);
  hw_initialized = true;

  // Luôn quét đủ BUFFER_SIZE bytes để đảm bảo tất cả IC trên chuỗi (kể cả IC mở rộng)
  // đều được cập nhật đúng và xóa sạch các chân không dùng đến.
  digitalWrite(PIN_LAT, LOW);
  delayMicroseconds(2);

  for (int8_t i = BUFFER_SIZE - 1; i >= 0; i--) {
    uint8_t b = led_buffer[i];
    for (int8_t bit = 7; bit >= 0; bit--) {
      bool bitVal = (b >> bit) & 0x01;
      digitalWrite(PIN_DATA, bitVal ? HIGH : LOW);
      delayMicroseconds(2);
      digitalWrite(PIN_CLK, HIGH);
      delayMicroseconds(2);
      digitalWrite(PIN_CLK, LOW);
      delayMicroseconds(2);
    }
  }

  // Chốt dữ liệu ra output của toàn bộ các IC 74HC595 đồng thời
  digitalWrite(PIN_LAT, HIGH);
  delayMicroseconds(5);
  digitalWrite(PIN_LAT, LOW);
  delayMicroseconds(2);
}

void shift595_set_pin(uint8_t pin, bool state, bool auto_update) {
  if (pin >= MAX_STEPS) return;
  uint8_t byte_idx = pin / 8;
  uint8_t bit_idx = pin % 8;

  if (state) {
    led_buffer[byte_idx] |= (1 << bit_idx);
  } else {
    led_buffer[byte_idx] &= ~(1 << bit_idx);
  }

  if (auto_update) {
    shift595_update();
  }
}

bool shift595_get_pin(uint8_t pin) {
  if (pin >= MAX_STEPS) return false;
  uint8_t byte_idx = pin / 8;
  uint8_t bit_idx = pin % 8;
  return (led_buffer[byte_idx] & (1 << bit_idx)) != 0;
}

void shift595_set_all(bool state) {
  if (!state) {
    memset(led_buffer, 0, sizeof(led_buffer));
  } else {
    memset(led_buffer, 0, sizeof(led_buffer));
    uint8_t N = config.number_step;
    if (N > MAX_STEPS) N = MAX_STEPS;
    for (uint8_t i = 0; i < N; i++) {
      led_buffer[i / 8] |= (1 << (i % 8));
    }
  }
  shift595_update();
}

void shift595_clear_all(void) {
  memset(led_buffer, 0, sizeof(led_buffer));
  shift595_update();
}

// Hàm tương thích ngược khi nhận mask 32-bit (dành cho các mã kiểm tra cũ)
void shift595_set_mask32(uint32_t mask, bool force) {
  memset(led_buffer, 0, sizeof(led_buffer));
  led_buffer[0] = mask & 0xFF;
  led_buffer[1] = (mask >> 8) & 0xFF;
  led_buffer[2] = (mask >> 16) & 0xFF;
  led_buffer[3] = (mask >> 24) & 0xFF;
  shift595_update(force);
}

// Quản lý EEPROM
void eeprom_reset_defaults(void) {
  config.magic = EEPROM_MAGIC;
  config.number_step = 25;       // Mặc định 25 bậc (hỗ trợ điều chỉnh 1 đến MAX_STEPS)
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
    if (config.mode < 1 || config.mode > 9) {
      config.mode = MODE_SMART_SENSOR;
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
