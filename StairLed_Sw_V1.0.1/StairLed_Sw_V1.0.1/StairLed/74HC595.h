#ifndef _74HC595_H
#define _74HC595_H

#include <Arduino.h>
#include <stdint.h>

// Định nghĩa chân kết nối phần cứng theo sơ đồ mạch ESP8266 (NodeMCU / WeMos D1)
#define PIN_LAT   D3  // Chân Latch (ST_CP / RCLK) - GPIO0
#define PIN_CLK   D4  // Chân Clock (SH_CP / SRCLK) - GPIO2
#define PIN_DATA  D7  // Chân Data (DS / SER) - GPIO13

// #define PIN_SS1   D5  // Cảm biến tiệm cận DƯỚI cầu thang (GPIO14)
// #define PIN_SS2   D6  // Cảm biến tiệm cận TRÊN cầu thang (GPIO12)

#define PIN_SS1   D2  // Cảm biến tiệm cận DƯỚI cầu thang (GPIO14)
#define PIN_SS2   D1  // Cảm biến tiệm cận TRÊN cầu thang (GPIO12)

#define PIN_BT1   D5  // Nút nhấn vật lý 1 (GPIO5)
#define PIN_BT2   D6  // Nút nhấn vật lý 2 (GPIO4)

// Cấu hình số bậc cầu thang tối đa: Cố định tối đa 27 bậc
#define MAX_STEPS 27
#define BUFFER_SIZE ((MAX_STEPS + 7) / 8) // Số byte bộ đệm (27 bậc = 4 bytes / 4 IC 74HC595)
#define EEPROM_MAGIC 0x5968  // Mã nhận diện cấu hình EEPROM hợp lệ (Cập nhật t0, t1, t2)

// Các chế độ hoạt động chính
enum RunMode {
  MODE_SMART_SENSOR = 1, // Chế độ cảm biến thông minh tự động (Mặc định: Kịch bản 1 -> 6)
  MODE_ALL_ON,           // Luôn sáng toàn bộ số bậc (Chế độ kiểm tra LED)
  MODE_SANG_DON_LEN,     // Demo: Sáng dồn lên -> giữ sáng t2 -> tắt dồn lên t1
  MODE_SANG_DON_XUONG,   // Demo: Sáng dồn xuống -> giữ sáng t2 -> tắt dồn xuống t1
  MODE_SAO_BANG,         // Demo: Hiệu ứng sao băng / rượt đuổi
  MODE_NHAP_NHAY,        // Demo: Hiệu ứng chớp nháy đồng bộ (Blink)
  MODE_SO_LE,            // Demo: Hiệu ứng sáng tắt so le chẵn lẻ
  MODE_ALL_OFF,          // Tắt toàn bộ
  MODE_AUTO_DEMO         // Chế độ 9: Chạy toàn bộ các chế độ
};

// Trạng thái máy hữu hạn FSM cho điều khiển LED cầu thang thông minh
enum StairState {
  STATE_IDLE = 0,        // Không có người, đèn tắt
  STATE_RUN_UP,          // Kịch bản 1: Đang sáng dần từ dưới lên (1 -> N)
  STATE_RUN_DOWN,        // Kịch bản 2: Đang sáng dần từ trên xuống (N -> 1)
  STATE_RUN_BOTH,        // Kịch bản 3: Đang sáng dần từ 2 phía vào giữa
  STATE_HOLD,            // Toàn bộ LED sáng, đang giữ sáng trong thời gian t2
  STATE_OFF_UP,          // Đang tắt dần từ dưới lên (1 -> N) với thời gian t1
  STATE_OFF_DOWN,        // Đang tắt dần từ trên xuống (N -> 1) với thời gian t1
  STATE_OFF_BOTH         // Đang tắt dần từ 2 phía vào giữa với thời gian t1
};

// Cấu trúc dữ liệu cấu hình lưu vào EEPROM
struct ConfigData {
  uint16_t magic;              // Magic number kiểm tra dữ liệu khởi tạo
  uint8_t  number_step;        // Số bậc cầu thang (tương ứng số LED, 1 đến MAX_STEPS)
  uint16_t t0;                 // Thời gian sáng từng bậc t0 (ms)
  uint16_t t1;                 // Thời gian tắt từng bậc t1 (ms)
  uint16_t t2;                 // Thời gian sáng sau khi rời cầu thang t2 (ms)
  uint8_t  mode;               // Chế độ hoạt động (RunMode)
};

extern ConfigData config;
extern uint8_t led_buffer[BUFFER_SIZE];

// Các hàm điều khiển 74HC595 hỗ trợ mở rộng số bậc tùy ý (Mức 1 sáng LED)
void shift595_init(void);
void shift595_update(bool force = false);
void shift595_set_pin(uint8_t pin, bool state, bool auto_update = true);
void shift595_set_all(bool state);
void shift595_clear_all(void);
bool shift595_get_pin(uint8_t pin);
void shift595_set_mask32(uint32_t mask, bool force = false); // Hàm tương thích 32-bit cũ

// Hàm cấu hình EEPROM
void eeprom_init_and_load(void);
void eeprom_save(void);
void eeprom_reset_defaults(void);

#endif // _74HC595_H
