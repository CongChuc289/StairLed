#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include "74HC595.h"
#include "webpage.h"

// Thông tin mạng WiFi Access Point (Cố định phần mềm, không đổi qua web)
const char* AP_SSID = "StairLed_CTECK";
const char* AP_PASS = "12345678";

// Cổng DNS cho Captive Portal
const byte DNS_PORT = 53;
DNSServer dnsServer;

// Web Server cổng 80
ESP8266WebServer server(80);

// Quản lý khởi động lại an toàn (Non-blocking Deferred Restart)
bool should_restart = false;
unsigned long restart_start_time = 0;

// Quản lý trạng thái máy FSM cầu thang
StairState stair_state = STATE_IDLE;
StairState next_off_state = STATE_OFF_UP; // Lưu hướng tắt dự kiến sau khi giữ sáng (HOLD)

int16_t step_idx = 0;
int16_t step_idx2 = 0;
int16_t fade_step = 0; // Biến đếm bước tắt dồn độc lập (1 -> N)
unsigned long last_step_time = 0;
unsigned long hold_start_time = 0;

// Bộ đệm đọc cảm biến với chống nhiễu (Debounce)
bool ss1_current = false;
bool ss2_current = false;
bool ss1_last_stable = false;
bool ss2_last_stable = false;
unsigned long ss1_last_debounce_time = 0;
unsigned long ss2_last_debounce_time = 0;
const unsigned long DEBOUNCE_DELAY_MS = 30; // 30ms lọc sạch nhiễu xung cảm biến

bool event_ss1_trigger = false;
bool event_ss2_trigger = false;

// Đọc nút nhấn vật lý BT1, BT2
bool bt1_last = HIGH;
bool bt2_last = HIGH;

// Biến quản lý chế độ Demo non-blocking
unsigned long demo_last_time = 0;
int16_t demo_step = 0;
uint8_t demo_phase = 0; // 0: Sáng dồn (t0), 1: Giữ sáng (t2), 2: Tắt dồn (t1), 3: Nghỉ
bool demo_flag = false;

// Khai báo hàm
void handleSensorInputs();
void handlePhysicalButtons();
void updateStairStateMachine();
void runDemoEffects();
void setupWebServer();

// Helper trích xuất JSON độc lập không dùng cấp phát động String (Zero-allocation, chống tràn bộ nhớ)
int getJsonInt(const char* json, const char* key, int default_val) {
  if (!json || !key) return default_val;
  const char* p = strstr(json, key);
  if (!p) return default_val;
  p += strlen(key);
  p = strchr(p, ':');
  if (!p) return default_val;
  p++;
  while (*p == ' ' || *p == '\t' || *p == '\"') {
    p++;
  }
  if (*p == '-' || (*p >= '0' && *p <= '9')) {
    return (int)strtol(p, NULL, 10);
  }
  return default_val;
}

bool getJsonStr(const char* json, const char* key, char* out_buf, size_t max_len) {
  if (!json || !key || !out_buf || max_len == 0) return false;
  out_buf[0] = '\0';
  const char* p = strstr(json, key);
  if (!p) return false;
  p += strlen(key);
  p = strchr(p, ':');
  if (!p) return false;
  p++;
  p = strchr(p, '\"');
  if (!p) return false;
  p++;
  const char* end_quote = strchr(p, '\"');
  if (!end_quote) return false;
  size_t len = (size_t)(end_quote - p);
  if (len >= max_len) len = max_len - 1;
  memcpy(out_buf, p, len);
  out_buf[len] = '\0';
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("\n=========================================="));
  Serial.println(F("  SMART STAIR LED CONTROLLER (STABLE 24/7)"));
  Serial.println(F("=========================================="));

  // Cấu hình chân nút bấm & cảm biến tiệm cận
  pinMode(PIN_SS1, INPUT_PULLUP);
  pinMode(PIN_SS2, INPUT_PULLUP);
  pinMode(PIN_BT1, INPUT_PULLUP);
  pinMode(PIN_BT2, INPUT_PULLUP);

  // Khởi tạo EEPROM và nạp cấu hình
  eeprom_init_and_load();

  // Khởi tạo phần cứng 74HC595
  shift595_init();

  // Tối ưu hóa WiFi SoftAP hoạt động liên tục 24/7:
  // - Vô hiệu hóa việc tự động ghi cấu hình WiFi vào Flash mỗi lần boot (bảo vệ chip nhớ)
  // - Tắt Sleep mode để SoftAP ổn định tuyệt đối, không rớt kết nối
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  IPAddress apIP(192, 168, 4, 1);
  IPAddress netMsk(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  // Giới hạn tối đa 4 client đồng thời nhằm bảo toàn bộ nhớ heap của stack mạng
  WiFi.softAP(AP_SSID, AP_PASS, 1, 0, 4);
  Serial.print(F("[WIFI] Access Point SSID: "));
  Serial.println(AP_SSID);
  Serial.print(F("[WIFI] IP Address: "));
  Serial.println(WiFi.softAPIP());
  Serial.printf_P(PSTR("[WIFI] Khởi động thành công! Free Heap: %u Bytes\n"), ESP.getFreeHeap());

  // Khởi động DNS Server để chuyển hướng mọi truy vấn tên miền về IP của ESP (Captive Portal)
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  Serial.println(F("[DNS] Captive Portal DNS Server ready"));

  // Thiết lập Web Server
  setupWebServer();
  server.begin();
  Serial.println(F("[WEB] Server port 80 ready!"));

  // Hiệu ứng khởi động nhẹ
  for (uint8_t i = 0; i < 3; i++) {
    for (uint8_t b = 0; b < 4 && b < config.number_step; b++) {
      shift595_set_pin(b, true, false);
    }
    shift595_update();
    delay(70);
    shift595_clear_all();
    delay(70);
  }
}

void loop() {
  // Xử lý Captive Portal DNS
  dnsServer.processNextRequest();

  // Xử lý yêu cầu HTTP từ Web (Non-blocking)
  server.handleClient();

  // Khởi động lại an toàn sau khi đã gửi xong phản hồi HTTP cho client
  if (should_restart && (millis() - restart_start_time >= 500)) {
    Serial.println(F("[SYSTEM] Khởi động lại vi điều khiển..."));
    WiFi.softAPdisconnect(true);
    delay(50);
    ESP.restart();
  }

  // Đọc và chống dội cảm biến tiệm cận
  handleSensorInputs();

  // Đọc nút bấm vật lý trên mạch
  handlePhysicalButtons();

  // Xử lý logic LED theo chế độ cấu hình
  if (config.mode == MODE_SMART_SENSOR) {
    updateStairStateMachine();
  } else {
    runDemoEffects();
  }

  // Báo cáo sức khỏe hệ thống định kỳ mỗi 60 giây (kiểm soát rò rỉ bộ nhớ / phân mảnh heap)
  static unsigned long last_health_report = 0;
  unsigned long now = millis();
  if (now - last_health_report >= 60000) {
    last_health_report = now;
    Serial.printf_P(PSTR("[SYS HEALTH] Uptime: %lu s | Free Heap: %u B | Max Block: %u B | Frag: %u%%\n"),
                    now / 1000, ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(), ESP.getHeapFragmentation());
  }

  // Cho phép ESP8266 xử lý ngầm TCP/IP và nuôi Watchdog Timer
  yield();
}

// ====================================================================
// 1. XỬ LÝ CẢM BIẾN TIỆM CẬN (SS1 - DƯỚI, SS2 - TRÊN)
//    Mức tín hiệu cố định phần cứng: Tín hiệu 1 (HIGH) khi có người
// ====================================================================
void handleSensorInputs() {
  // Cố định mức tín hiệu phần cứng: Mức 1 (HIGH) khi phát hiện người
  bool reading1 = (digitalRead(PIN_SS1) == HIGH);
  bool reading2 = (digitalRead(PIN_SS2) == HIGH);

  unsigned long now = millis();

  // Debounce SS1 (Cảm biến dưới)
  static bool last_raw1 = false;
  if (reading1 != last_raw1) {
    ss1_last_debounce_time = now;
    last_raw1 = reading1;
  }
  if ((now - ss1_last_debounce_time) > DEBOUNCE_DELAY_MS) {
    if (reading1 != ss1_last_stable) {
      ss1_last_stable = reading1;
      if (ss1_last_stable) {
        event_ss1_trigger = true;
        Serial.println(F("[SENSOR] -> SS1 (DƯỚI) PHÁT HIỆN NGƯỜI!"));
      }
    }
  }
  ss1_current = ss1_last_stable;

  // Debounce SS2 (Cảm biến trên)
  static bool last_raw2 = false;
  if (reading2 != last_raw2) {
    ss2_last_debounce_time = now;
    last_raw2 = reading2;
  }
  if ((now - ss2_last_debounce_time) > DEBOUNCE_DELAY_MS) {
    if (reading2 != ss2_last_stable) {
      ss2_last_stable = reading2;
      if (ss2_last_stable) {
        event_ss2_trigger = true;
        Serial.println(F("[SENSOR] -> SS2 (TRÊN) PHÁT HIỆN NGƯỜI!"));
      }
    }
  }
  ss2_current = ss2_last_stable;
}

// ====================================================================
// 2. NÚT NHẤN VẬT LÝ TRÊN MẠCH (BT1, BT2)
// ====================================================================
void handlePhysicalButtons() {
  bool bt1 = digitalRead(PIN_BT1);
  bool bt2 = digitalRead(PIN_BT2);

  if (bt1 == LOW && bt1_last == HIGH) {
    event_ss1_trigger = true;
    Serial.println(F("[BUTTON] Bấm BT1 -> Test đi lên (SS1)"));
  }
  if (bt2 == LOW && bt2_last == HIGH) {
    event_ss2_trigger = true;
    Serial.println(F("[BUTTON] Bấm BT2 -> Test đi xuống (SS2)"));
  }

  bt1_last = bt1;
  bt2_last = bt2;
}

// ====================================================================
// 3. MÁY TRẠNG THÁI FSM ĐIỀU KHIỂN CẦU THANG (KỊCH BẢN 1 ĐẾN 6)
// ====================================================================
void cancelFadeOffAndRunUp() {
  uint8_t N = config.number_step;
  if (N == 0 || N > MAX_STEPS) N = MAX_STEPS;
  uint16_t t0 = (config.t0 >= 20 && config.t0 <= 5000) ? config.t0 : 150;
  stair_state = STATE_RUN_UP;
  step_idx = 0;
  fade_step = 0;
  shift595_clear_all();
  shift595_set_pin(step_idx, true);
  last_step_time = millis();
  next_off_state = STATE_OFF_UP;
  Serial.printf_P(PSTR("[FSM] -> HỦY TẮT DẦN: Người mới đi lên -> Sáng dồn từ dưới lên (Bậc 1/%u, t0=%ums)\n"), N, t0);
}

void cancelFadeOffAndRunDown() {
  uint8_t N = config.number_step;
  if (N == 0 || N > MAX_STEPS) N = MAX_STEPS;
  uint16_t t0 = (config.t0 >= 20 && config.t0 <= 5000) ? config.t0 : 150;
  stair_state = STATE_RUN_DOWN;
  step_idx = N - 1;
  fade_step = 0;
  shift595_clear_all();
  shift595_set_pin(step_idx, true);
  last_step_time = millis();
  next_off_state = STATE_OFF_DOWN;
  Serial.printf_P(PSTR("[FSM] -> HỦY TẮT DẦN: Người mới đi xuống -> Sáng dồn từ trên xuống (Bậc %u/%u, t0=%ums)\n"), N, N, t0);
}

void cancelFadeOffAndRunBoth() {
  uint8_t N = config.number_step;
  if (N == 0 || N > MAX_STEPS) N = MAX_STEPS;
  uint16_t t0 = (config.t0 >= 20 && config.t0 <= 5000) ? config.t0 : 150;
  stair_state = STATE_RUN_BOTH;
  step_idx = 0;
  step_idx2 = N - 1;
  fade_step = 0;
  shift595_clear_all();
  shift595_set_pin(step_idx, true, false);
  shift595_set_pin(step_idx2, true, true);
  last_step_time = millis();
  next_off_state = STATE_OFF_BOTH;
  Serial.printf_P(PSTR("[FSM] -> HỦY TẮT DẦN: Cả 2 hướng cùng đi -> Sáng dồn từ 2 phía vào giữa (t0=%ums)\n"), t0);
}

void updateStairStateMachine() {
  unsigned long now = millis();
  uint8_t N = config.number_step;
  if (N == 0 || N > MAX_STEPS) N = MAX_STEPS;

  // Đảm bảo các mốc thời gian luôn có giá trị an toàn:
  // t0: Thời gian sáng từng bậc (ms)
  // t1: Thời gian tắt từng bậc (ms)
  // t2: Thời gian sáng sau khi rời cầu thang (ms)
  uint16_t t0 = (config.t0 >= 20 && config.t0 <= 5000) ? config.t0 : 150;
  uint16_t t1 = (config.t1 >= 20 && config.t1 <= 5000) ? config.t1 : 150;
  uint16_t t2 = (config.t2 >= 500 && config.t2 <= 60000) ? config.t2 : 5000;

  // Thu thập sự kiện cảm biến và reset cờ sau khi nhận diện
  bool trig1 = event_ss1_trigger;
  bool trig2 = event_ss2_trigger;
  event_ss1_trigger = false;
  event_ss2_trigger = false;

  switch (stair_state) {
    // -------------------------------------------------------------
    // TRẠNG THÁI CHỜ: KHÔNG CÓ NGƯỜI
    // -------------------------------------------------------------
    case STATE_IDLE: {
      if (trig1 && trig2) {
        // TRƯỜNG HỢP 3: Cả hai cảm biến cùng phát hiện
        Serial.println(F("[FSM] -> KÍCH HOẠT TRƯỜNG HỢP 3: Cả 2 cảm biến cùng phát hiện!"));
        stair_state = STATE_RUN_BOTH;
        step_idx = 0;
        step_idx2 = N - 1;
        fade_step = 0;
        shift595_clear_all();
        shift595_set_pin(step_idx, true, false);
        shift595_set_pin(step_idx2, true, true);
        last_step_time = now;
        next_off_state = STATE_OFF_BOTH;
      } else if (trig1) {
        // TRƯỜNG HỢP 1: Đi từ dưới lên (SS1 = 1, SS2 = 0)
        Serial.println(F("[FSM] -> KÍCH HOẠT TRƯỜNG HỢP 1: Đi từ dưới lên!"));
        stair_state = STATE_RUN_UP;
        step_idx = 0;
        fade_step = 0;
        shift595_clear_all();
        shift595_set_pin(step_idx, true);
        Serial.printf_P(PSTR("[FSM] -> Bậc 1/%u sáng (t0=%ums)\n"), N, t0);
        last_step_time = now;
        next_off_state = STATE_OFF_UP;
      } else if (trig2) {
        // TRƯỜNG HỢP 2: Đi từ trên xuống (SS1 = 0, SS2 = 1)
        Serial.println(F("[FSM] -> KÍCH HOẠT TRƯỜNG HỢP 2: Đi từ trên xuống!"));
        stair_state = STATE_RUN_DOWN;
        step_idx = N - 1;
        fade_step = 0;
        shift595_clear_all();
        shift595_set_pin(step_idx, true);
        Serial.printf_P(PSTR("[FSM] -> Bậc %u/%u sáng (t0=%ums)\n"), N, N, t0);
        last_step_time = now;
        next_off_state = STATE_OFF_DOWN;
      }
      break;
    }

    // -------------------------------------------------------------
    // TRƯỜNG HỢP 1: ĐANG SÁNG DỒN TỪ DƯỚI LÊN (1 -> N, mỗi bậc t0)
    // -------------------------------------------------------------
    case STATE_RUN_UP: {
      if (now - last_step_time >= t0) {
        last_step_time = now;
        step_idx++;
        if (step_idx < N) {
          shift595_set_pin(step_idx, true);
          Serial.printf_P(PSTR("[FSM] -> Bậc %u/%u sáng (t0=%ums)\n"), step_idx + 1, N, t0);
        } else {
          // Đã sáng đủ N bậc đi lên -> Chuyển sang GIỮ SÁNG t2 (HOLD)
          shift595_set_all(true);
          stair_state = STATE_HOLD;
          hold_start_time = now;
          next_off_state = STATE_OFF_UP;
          Serial.printf_P(PSTR("[FSM] -> Sáng đủ %u bậc đi lên -> Chuyển sang GIỮ SÁNG t2=%ums (HOLD)\n"), N, t2);
        }
      }
      break;
    }

    // -------------------------------------------------------------
    // TRƯỜNG HỢP 2: ĐANG SÁNG DỒN TỪ TRÊN XUỐNG (N -> 1, mỗi bậc t0)
    // -------------------------------------------------------------
    case STATE_RUN_DOWN: {
      if (now - last_step_time >= t0) {
        last_step_time = now;
        step_idx--;
        if (step_idx >= 0) {
          shift595_set_pin(step_idx, true);
          Serial.printf_P(PSTR("[FSM] -> Bậc %u/%u sáng (t0=%ums)\n"), step_idx + 1, N, t0);
        } else {
          // Đã sáng đủ N bậc đi xuống -> Chuyển sang GIỮ SÁNG t2 (HOLD)
          shift595_set_all(true);
          stair_state = STATE_HOLD;
          hold_start_time = now;
          next_off_state = STATE_OFF_DOWN;
          Serial.printf_P(PSTR("[FSM] -> Sáng đủ %u bậc đi xuống -> Chuyển sang GIỮ SÁNG t2=%ums (HOLD)\n"), N, t2);
        }
      }
      break;
    }

    // -------------------------------------------------------------
    // TRƯỜNG HỢP 3: ĐANG SÁNG DỒN TỪ CẢ HAI HƯỚNG VÀO TRUNG TÂM (mỗi bậc t0)
    // -------------------------------------------------------------
    case STATE_RUN_BOTH: {
      if (now - last_step_time >= t0) {
        last_step_time = now;
        step_idx++;
        step_idx2--;

        if (step_idx < N) shift595_set_pin(step_idx, true, false);
        if (step_idx2 >= 0) shift595_set_pin(step_idx2, true, false);
        shift595_update();

        // Hai luồng sáng gặp nhau ở trung tâm cầu thang
        if (step_idx >= step_idx2) {
          shift595_set_all(true);
          stair_state = STATE_HOLD;
          hold_start_time = now;
          next_off_state = STATE_OFF_BOTH;
          Serial.println(F("[FSM] -> Sáng chạm LED trung tâm -> Chuyển sang GIỮ SÁNG t2 (HOLD)"));
        }
      }
      break;
    }

    // -------------------------------------------------------------
    // TRẠNG THÁI GIỮ SÁNG TOÀN BỘ (HOLD) TRONG THỜI GIAN t2
    // -------------------------------------------------------------
    case STATE_HOLD: {
      // Chỉ gia hạn t2 nếu có người MỚI bước vào từ lối vào (không tính cảm biến ở lối ra)
      if ((next_off_state == STATE_OFF_UP && trig1) ||
          (next_off_state == STATE_OFF_DOWN && trig2) ||
          (trig1 && trig2)) {
        hold_start_time = now;
        Serial.println(F("[FSM] -> Có người mới bước vào lối vào khi đang giữ sáng -> Gia hạn thêm thời gian t2!"));
        if (trig1 && trig2) {
          next_off_state = STATE_OFF_BOTH;
        }
      }

      // Hết thời gian giữ sáng t2 -> Chuyển sang TẮT DỒN theo hướng tương ứng với thời gian t1
      if (now - hold_start_time >= t2) {
        stair_state = next_off_state;
        last_step_time = now;
        fade_step = 0; // Khởi tạo bước tắt dồn từ 0

        // Đảm bảo toàn bộ LED đang sáng đủ trước khi bắt đầu chu trình tắt dồn
        shift595_set_all(true);

        if (stair_state == STATE_OFF_UP) {
          Serial.println(F("[FSM] -> Hết t2: Bắt đầu TẮT LẦN LƯỢT TỪ DƯỚI LÊN (1 -> N, mỗi bậc tắt sau thời gian t1)"));
        } else if (stair_state == STATE_OFF_DOWN) {
          Serial.println(F("[FSM] -> Hết t2: Bắt đầu TẮT LẦN LƯỢT TỪ TRÊN XUỐNG (N -> 1, mỗi bậc tắt sau thời gian t1)"));
        } else {
          Serial.println(F("[FSM] -> Hết t2: Bắt đầu TẮT LẦN LƯỢT TỪ 2 PHÍA VÀO GIỮA (mỗi bậc tắt sau thời gian t1)"));
        }
      }
      break;
    }

    // -------------------------------------------------------------
    // TRƯỜNG HỢP 4: LED TẮT LẦN LƯỢT TỪ DƯỚI LÊN (1 -> N, mỗi bậc tắt sau thời gian t1)
    // -------------------------------------------------------------
    case STATE_OFF_UP: {
      if (trig1 && trig2) {
        cancelFadeOffAndRunBoth();
        break;
      } else if (trig1) {
        cancelFadeOffAndRunUp();
        break;
      } else if (trig2) {
        cancelFadeOffAndRunDown();
        break;
      }

      if (now - last_step_time >= t1) {
        last_step_time = now;
        fade_step++;

        if (fade_step <= N) {
          // Tắt trực tiếp bit của bậc tương ứng từ dưới lên (bậc 1 tương ứng index 0)
          shift595_set_pin(fade_step - 1, false);

          if (fade_step < N) {
            Serial.printf_P(PSTR("[FSM] -> Bậc %u/%u TẮT (sau thời gian t1=%ums, còn %u bậc đang sáng)\n"), fade_step, N, t1, N - fade_step);
          } else {
            // Đã tắt đến bậc cuối cùng N
            shift595_set_all(false);
            stair_state = STATE_IDLE;
            fade_step = 0;
            Serial.printf_P(PSTR("[FSM] -> Bậc %u/%u (bậc cuối cùng) TẮT -> Hoàn tất tắt lần lượt từ dưới lên, trở về trạng thái CHỜ (IDLE)\n"), N, N);
          }
        }
      }
      break;
    }

    // -------------------------------------------------------------
    // TRƯỜNG HỢP 5: LED TẮT LẦN LƯỢT TỪ TRÊN XUỐNG (N -> 1, mỗi bậc tắt sau thời gian t1)
    // -------------------------------------------------------------
    case STATE_OFF_DOWN: {
      if (trig1 && trig2) {
        cancelFadeOffAndRunBoth();
        break;
      } else if (trig1) {
        cancelFadeOffAndRunUp();
        break;
      } else if (trig2) {
        cancelFadeOffAndRunDown();
        break;
      }

      if (now - last_step_time >= t1) {
        last_step_time = now;
        fade_step++;

        if (fade_step <= N) {
          // Tắt trực tiếp bit của bậc tương ứng từ trên xuống (bậc N tương ứng index N-1)
          shift595_set_pin(N - fade_step, false);

          if (fade_step < N) {
            Serial.printf_P(PSTR("[FSM] -> Bậc %u/%u TẮT (sau thời gian t1=%ums, còn %u bậc đang sáng)\n"), N - fade_step + 1, N, t1, N - fade_step);
          } else {
            // Đã tắt đến bậc cuối cùng 1
            shift595_set_all(false);
            stair_state = STATE_IDLE;
            fade_step = 0;
            Serial.printf_P(PSTR("[FSM] -> Bậc 1/%u (bậc cuối cùng) TẮT -> Hoàn tất tắt lần lượt từ trên xuống, trở về trạng thái CHỜ (IDLE)\n"), N);
          }
        }
      }
      break;
    }

    // -------------------------------------------------------------
    // TRƯỜNG HỢP 6: LED TẮT LẦN LƯỢT TỪ 2 PHÍA VÀO GIỮA (mỗi bậc tắt sau thời gian t1)
    // -------------------------------------------------------------
    case STATE_OFF_BOTH: {
      if (trig1 && trig2) {
        cancelFadeOffAndRunBoth();
        break;
      } else if (trig1) {
        cancelFadeOffAndRunUp();
        break;
      } else if (trig2) {
        cancelFadeOffAndRunDown();
        break;
      }

      if (now - last_step_time >= t1) {
        last_step_time = now;
        fade_step++;

        int16_t left_bit = fade_step - 1;
        int16_t right_bit = N - fade_step;

        if (left_bit <= right_bit) {
          shift595_set_pin(left_bit, false, false);
          shift595_set_pin(right_bit, false, false);
          shift595_update();

          if (left_bit < right_bit) {
            Serial.printf_P(PSTR("[FSM] -> Bậc %u và bậc %u TẮT (sau thời gian t1=%ums, còn %u bậc đang sáng)\n"), left_bit + 1, right_bit + 1, t1, right_bit - left_bit - 1);
          } else {
            Serial.printf_P(PSTR("[FSM] -> Bậc tâm %u TẮT (sau thời gian t1=%ums)\n"), left_bit + 1, t1);
          }
        } else {
          shift595_set_all(false);
          stair_state = STATE_IDLE;
          fade_step = 0;
          Serial.println(F("[FSM] -> Hoàn tất tắt lần lượt 2 phía vào giữa, trở về trạng thái CHỜ (IDLE)"));
        }
      }
      break;
    }
  }
}

// ====================================================================
// 4. HIỆU ỨNG DEMO (KHI KHÔNG DÙNG CẢM BIẾN TỰ ĐỘNG) - NON-BLOCKING
// ====================================================================
void runDemoEffects() {
  unsigned long now = millis();
  uint8_t N = config.number_step;
  if (N == 0 || N > MAX_STEPS) N = 25;

  static uint8_t last_demo_mode = 0;
  if (config.mode != last_demo_mode) {
    last_demo_mode = config.mode;
    demo_phase = 0;
    demo_step = 0;
    demo_last_time = now;
    demo_flag = false;
    shift595_clear_all();
  }

  uint16_t t0 = (config.t0 >= 20 && config.t0 <= 5000) ? config.t0 : 150;
  uint16_t t1 = (config.t1 >= 20 && config.t1 <= 5000) ? config.t1 : 150;
  uint16_t t2 = (config.t2 >= 500 && config.t2 <= 60000) ? config.t2 : 5000;

  switch (config.mode) {
    case MODE_ALL_ON:
      shift595_set_all(true);
      break;

    case MODE_ALL_OFF:
      shift595_set_all(false);
      break;

    case MODE_SANG_DON_LEN: {
      // Chu trình: Sáng dồn từ dưới lên (t0) -> Giữ sáng (t2) -> Tắt dồn từ dưới lên (t1) -> Lặp lại
      if (demo_phase == 0) {
        // Giai đoạn 0: Sáng dồn từ dưới lên (1 -> N, mỗi bậc t0)
        if (now - demo_last_time >= t0) {
          demo_last_time = now;
          demo_step++;
          if (demo_step <= N) {
            shift595_set_pin(demo_step - 1, true);
          }
          if (demo_step >= N) {
            shift595_set_all(true);
            demo_phase = 1; // Chuyển sang Giữ sáng t2
            demo_last_time = now;
          }
        }
      } else if (demo_phase == 1) {
        // Giai đoạn 1: Giữ sáng toàn bộ trong thời gian t2
        if (now - demo_last_time >= t2) {
          demo_phase = 2; // Chuyển sang Tắt dồn từ dưới lên
          demo_step = 0;
          demo_last_time = now;
        }
      } else if (demo_phase == 2) {
        // Giai đoạn 2: Tắt dồn từ dưới lên (1 -> N, mỗi bậc t1)
        if (now - demo_last_time >= t1) {
          demo_last_time = now;
          demo_step++;
          if (demo_step <= N) {
            shift595_set_pin(demo_step - 1, false);
          }
          if (demo_step >= N) {
            shift595_set_all(false);
            demo_phase = 3; // Nghỉ tắt
            demo_last_time = now;
          }
        }        
      } else if (demo_phase == 3) {
        // Giai đoạn 3: Nghỉ tắt 500ms trước khi lặp lại chu trình
        if (now - demo_last_time >= 500) {
          demo_phase = 0;
          demo_step = 0;
          demo_last_time = now;
          shift595_clear_all();
        }
      }
      break;
    }

    case MODE_SANG_DON_XUONG: {
      // Chu trình: Sáng dồn từ trên xuống (t0) -> Giữ sáng (t2) -> Tắt dồn từ trên xuống (t1) -> Lặp lại
      if (demo_phase == 0) {
        // Giai đoạn 0: Sáng dồn từ trên xuống (N -> 1, mỗi bậc t0)
        if (now - demo_last_time >= t0) {
          demo_last_time = now;
          demo_step++;
          if (demo_step <= N) {
            shift595_set_pin(N - demo_step, true);
          }
          if (demo_step >= N) {
            shift595_set_all(true);
            demo_phase = 1; // Chuyển sang Giữ sáng t2
            demo_last_time = now;
          }
        }
      } else if (demo_phase == 1) {
        // Giai đoạn 1: Giữ sáng toàn bộ trong thời gian t2
        if (now - demo_last_time >= t2) {
          demo_phase = 2; // Chuyển sang Tắt dồn từ trên xuống
          demo_step = 0;
          demo_last_time = now;
        }
      } else if (demo_phase == 2) {
        // Giai đoạn 2: Tắt dồn từ trên xuống (N -> 1, mỗi bậc t1)
        if (now - demo_last_time >= t1) {
          demo_last_time = now;
          demo_step++;
          if (demo_step <= N) {
            shift595_set_pin(N - demo_step, false);
          }
          if (demo_step >= N) {
            shift595_set_all(false);
            demo_phase = 3; // Nghỉ tắt
            demo_last_time = now;
          }
        }
      } else if (demo_phase == 3) {
        // Giai đoạn 3: Nghỉ tắt 500ms trước khi lặp lại chu trình
        if (now - demo_last_time >= 500) {
          demo_phase = 0;
          demo_step = 0;
          demo_last_time = now;
          shift595_clear_all();
        }
      }
      break;
    }

    case MODE_SAO_BANG: {
      // Hiệu ứng Sao băng / Rượt đuổi tự động căn chỉnh thời gian tối ưu đẹp nhất:
      // - Tốc độ lướt: 45ms/bậc (vệt sáng lướt vun vút, mượt mà, không giật)
      // - Chiều dài vệt đuôi: 4-5 LED (tỷ lệ chuẩn theo số bậc)
      // - Khoảng nghỉ sau mỗi vệt: 450ms (tạo khoảng lặng trước khi đợt sao băng mới xẹt qua)
      const uint16_t METEOR_SPEED = 45;       // 45ms mỗi bước dịch chuyển
      const uint16_t METEOR_PAUSE = 450;      // 450ms khoảng nghỉ giữa các đợt sao băng
      uint8_t tail_len = (N >= 20) ? 5 : 4;   // Chiều dài đuôi sao băng

      if (demo_phase == 0) {
        // Giai đoạn sao băng đang lướt qua cầu thang
        if (now - demo_last_time >= METEOR_SPEED) {
          demo_last_time = now;
          demo_step++;

          if (demo_step < N + tail_len) {
            memset(led_buffer, 0, sizeof(led_buffer));
            for (uint8_t k = 0; k < tail_len; k++) {
              int pos = demo_step - k;
              if (pos >= 0 && pos < N) {
                led_buffer[pos / 8] |= (1 << (pos % 8));
              }
            }
            shift595_update();
          } else {
            // Sao băng đã bay hết cầu thang -> Tắt toàn bộ và chuyển sang giai đoạn nghỉ ngắn
            shift595_set_all(false);
            demo_phase = 1;
            demo_last_time = now;
          }
        }
      } else {
        // Giai đoạn nghỉ 450ms giữa 2 đợt sao băng
        if (now - demo_last_time >= METEOR_PAUSE) {
          demo_phase = 0;
          demo_step = 0;
          demo_last_time = now;
        }
      }
      break;
    }

    case MODE_NHAP_NHAY: {
      // Chớp nháy đồng bộ toàn bộ bậc
      if (now - demo_last_time >= 350) {
        demo_last_time = now;
        demo_flag = !demo_flag;
        shift595_set_all(demo_flag);
      }
      break;
    }

    case MODE_SO_LE: {
      // Hiệu ứng sáng, tắt so le chẵn - lẻ (chạy chuẩn trên mọi số bậc N)
      if (now - demo_last_time >= (t0 < 100 ? 250 : t0 * 2)) {
        demo_last_time = now;
        demo_flag = !demo_flag;
        memset(led_buffer, 0, sizeof(led_buffer));
        for (uint8_t i = 0; i < N; i++) {
          bool on = demo_flag ? (i % 2 == 0) : (i % 2 != 0);
          if (on) {
            led_buffer[i / 8] |= (1 << (i % 8));
          }
        }
        shift595_update();
      }
      break;
    }

    case MODE_AUTO_DEMO: {
      // Chế độ 9: Chạy toàn bộ các chế độ (không dùng cảm biến)
      // Thứ tự tuần hoàn:
      //   1. Sáng dồn từ dưới lên -> Giữ sáng -> Tắt dồn
      //   2. Sáng dồn từ trên xuống -> Giữ sáng -> Tắt dồn
      //   3. Sao băng / rượt đuổi (3 đợt lướt qua)
      //   4. Sáng tắt so le chẵn - lẻ (8 nhịp đảo chiều)
      //   5. Chớp nháy đồng bộ (6 nhịp nhấp nháy)
      //   6. Sáng toàn bộ 2.5 giây -> Quay vòng lại bước 1
      static uint8_t auto_sub = 0;
      static uint8_t auto_cnt = 0;

      static uint8_t last_check_mode = 0;
      if (last_check_mode != config.mode) {
        last_check_mode = config.mode;
        auto_sub = 0;
        auto_cnt = 0;
        demo_phase = 0;
        demo_step = 0;
        demo_last_time = now;
        demo_flag = false;
        shift595_clear_all();
      }

      switch (auto_sub) {
        // --- 1. Sáng dồn từ dưới lên (1 chu kỳ) ---
        case 0: {
          if (demo_phase == 0) {
            if (now - demo_last_time >= t0) {
              demo_last_time = now;
              demo_step++;
              if (demo_step <= N) shift595_set_pin(demo_step - 1, true);
              if (demo_step >= N) {
                shift595_set_all(true);
                demo_phase = 1;
                demo_last_time = now;
              }
            }
          } else if (demo_phase == 1) {
            uint16_t hold_time = (t2 > 2500) ? 2500 : t2;
            if (now - demo_last_time >= hold_time) {
              demo_phase = 2;
              demo_step = 0;
              demo_last_time = now;
            }
          } else if (demo_phase == 2) {
            if (now - demo_last_time >= t1) {
              demo_last_time = now;
              demo_step++;
              if (demo_step <= N) shift595_set_pin(demo_step - 1, false);
              if (demo_step >= N) {
                shift595_set_all(false);
                demo_phase = 3;
                demo_last_time = now;
              }
            }
          } else if (demo_phase == 3) {
            if (now - demo_last_time >= 400) {
              auto_sub = 1;
              demo_phase = 0;
              demo_step = 0;
              demo_last_time = now;
              shift595_clear_all();
            }
          }
          break;
        }

        // --- 2. Sáng dồn từ trên xuống (1 chu kỳ) ---
        case 1: {
          if (demo_phase == 0) {
            if (now - demo_last_time >= t0) {
              demo_last_time = now;
              demo_step++;
              if (demo_step <= N) shift595_set_pin(N - demo_step, true);
              if (demo_step >= N) {
                shift595_set_all(true);
                demo_phase = 1;
                demo_last_time = now;
              }
            }
          } else if (demo_phase == 1) {
            uint16_t hold_time = (t2 > 2500) ? 2500 : t2;
            if (now - demo_last_time >= hold_time) {
              demo_phase = 2;
              demo_step = 0;
              demo_last_time = now;
            }
          } else if (demo_phase == 2) {
            if (now - demo_last_time >= t1) {
              demo_last_time = now;
              demo_step++;
              if (demo_step <= N) shift595_set_pin(N - demo_step, false);
              if (demo_step >= N) {
                shift595_set_all(false);
                demo_phase = 3;
                demo_last_time = now;
              }
            }
          } else if (demo_phase == 3) {
            if (now - demo_last_time >= 400) {
              auto_sub = 2;
              auto_cnt = 0;
              demo_phase = 0;
              demo_step = 0;
              demo_last_time = now;
              shift595_clear_all();
            }
          }
          break;
        }

        // --- 3. Sao băng / rượt đuổi (3 đợt lướt qua) ---
        case 2: {
          const uint16_t METEOR_SPEED = 45;
          const uint16_t METEOR_PAUSE = 400;
          uint8_t tail_len = (N >= 20) ? 5 : 4;

          if (demo_phase == 0) {
            if (now - demo_last_time >= METEOR_SPEED) {
              demo_last_time = now;
              demo_step++;

              if (demo_step < N + tail_len) {
                memset(led_buffer, 0, sizeof(led_buffer));
                for (uint8_t k = 0; k < tail_len; k++) {
                  int pos = demo_step - k;
                  if (pos >= 0 && pos < N) {
                    led_buffer[pos / 8] |= (1 << (pos % 8));
                  }
                }
                shift595_update();
              } else {
                shift595_set_all(false);
                demo_phase = 1;
                demo_last_time = now;
                auto_cnt++;
              }
            }
          } else {
            if (now - demo_last_time >= METEOR_PAUSE) {
              if (auto_cnt >= 3) {
                auto_sub = 3;
                auto_cnt = 0;
                demo_phase = 0;
                demo_step = 0;
                demo_last_time = now;
                demo_flag = false;
                shift595_clear_all();
              } else {
                demo_phase = 0;
                demo_step = 0;
                demo_last_time = now;
              }
            }
          }
          break;
        }

        // --- 4. Sáng tắt so le chẵn - lẻ (8 nhịp đổi bên) ---
        case 3: {
          uint16_t rate = (t0 < 100) ? 250 : (t0 > 400 ? 400 : t0 * 2);
          if (now - demo_last_time >= rate) {
            demo_last_time = now;
            demo_flag = !demo_flag;
            memset(led_buffer, 0, sizeof(led_buffer));
            for (uint8_t i = 0; i < N; i++) {
              bool on = demo_flag ? (i % 2 == 0) : (i % 2 != 0);
              if (on) {
                led_buffer[i / 8] |= (1 << (i % 8));
              }
            }
            shift595_update();
            auto_cnt++;

            if (auto_cnt >= 8) {
              auto_sub = 4;
              auto_cnt = 0;
              demo_phase = 0;
              demo_step = 0;
              demo_last_time = now;
              demo_flag = false;
              shift595_clear_all();
            }
          }
          break;
        }

        // --- 5. Chớp nháy đồng bộ (6 nhịp) ---
        case 4: {
          if (now - demo_last_time >= 350) {
            demo_last_time = now;
            demo_flag = !demo_flag;
            shift595_set_all(demo_flag);
            auto_cnt++;

            if (auto_cnt >= 6) {
              auto_sub = 5;
              auto_cnt = 0;
              demo_last_time = now;
              shift595_set_all(true);
            }
          }
          break;
        }

        // --- 6. Sáng toàn bộ 2.5 giây rồi quay vòng lại từ đầu ---
        case 5: {
          if (now - demo_last_time >= 2500) {
            auto_sub = 0;
            auto_cnt = 0;
            demo_phase = 0;
            demo_step = 0;
            demo_last_time = now;
            demo_flag = false;
            shift595_clear_all();
          }
          break;
        }
      }
      break;
    }
  }
}

// ====================================================================
// 5. CÁC ĐƯỜNG DẪN WEB SERVER & CAPTIVE PORTAL
// ====================================================================

// Kiểm tra xem chuỗi host có phải là địa chỉ IP không (loại bỏ port nếu có) - Zero-allocation
bool isIpAddress(const char* str) {
  if (!str) return false;
  while (*str && *str != ':') {
    if (*str != '.' && (*str < '0' || *str > '9')) {
      return false;
    }
    str++;
  }
  return true;
}

// Xử lý Captive Portal chuyển hướng tự động khi kết nối vào mạng WiFi
bool handleCaptivePortal() {
  String host = server.hostHeader();
  // Nếu host không phải địa chỉ IP 192.168.4.1 của ESP
  if (!isIpAddress(host.c_str()) && host.indexOf(F("192.168.4.1")) < 0) {
    server.sendHeader(F("Location"), F("http://192.168.4.1/"), true);
    server.sendHeader(F("Connection"), F("close"));
    server.send_P(302, PSTR("text/plain"), PSTR(""));
    return true;
  }
  return false;
}

void handleRoot() {
  server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  server.sendHeader(F("Pragma"), F("no-cache"));
  server.sendHeader(F("Expires"), F("-1"));
  server.sendHeader(F("Connection"), F("close"));
  server.send_P(200, PSTR("text/html; charset=utf-8"), webpage);
}

// Tự động chuyển hướng hoặc mở ngay webpage khi người dùng nhập địa chỉ IP hoặc đường dẫn bất kỳ
void handleNotFound() {
  if (handleCaptivePortal()) {
    return;
  }

  const char* uri = server.uri().c_str();
  // Xử lý các probe endpoint kiểm tra kết nối mạng của Android, iOS, Windows -> Phản hồi 302 gọn nhẹ, không tốn buffer gửi HTML 14KB
  if (strstr(uri, "generate_204") || strstr(uri, "gen_204") ||
      strstr(uri, "hotspot-detect") || strstr(uri, "canonical") ||
      strstr(uri, "connecttest") || strstr(uri, "ncsi")) {
    server.sendHeader(F("Location"), F("http://192.168.4.1/"), true);
    server.sendHeader(F("Connection"), F("close"));
    server.send_P(302, PSTR("text/plain"), PSTR(""));
    return;
  }

  // Tự động mở webpage cho tất cả các request không khớp
  handleRoot();
}

void handleApiGetConfig() {
  // Trả về JSON bằng stack buffer, không cấp phát heap động, chống phân mảnh RAM
  char buf[256];
  snprintf_P(buf, sizeof(buf),
    PSTR("{\"mode\":%u,\"number_step\":%u,\"t0\":%u,\"t1\":%u,\"t2\":%u,"
         "\"t0_step_on\":%u,\"t0_step_off\":%u,\"t1_hold\":%u,"
         "\"free_heap\":%u,\"heap_frag\":%u}"),
    config.mode, config.number_step, config.t0, config.t1, config.t2,
    config.t0, config.t1, config.t2,
    ESP.getFreeHeap(), ESP.getHeapFragmentation()
  );

  server.sendHeader(F("Connection"), F("close"));
  server.send_P(200, PSTR("application/json"), buf);
}

void handleApiPostConfig() {
  bool shouldReboot = false;

  if (server.hasArg("plain")) {
    const char* body = server.arg("plain").c_str();

    int mode = getJsonInt(body, "mode", -1);
    if (mode >= 1 && mode <= 9) config.mode = mode;

    int steps = getJsonInt(body, "number_step", -1);
    if (steps >= 1 && steps <= MAX_STEPS) config.number_step = steps;

    // t0: Thời gian sáng từng bậc
    int t0_val = getJsonInt(body, "t0", -1);
    if (t0_val < 0) t0_val = getJsonInt(body, "t0_step_on", -1);
    if (t0_val >= 20 && t0_val <= 5000) config.t0 = t0_val;

    // t1: Thời gian tắt từng bậc
    int t1_val = getJsonInt(body, "t1", -1);
    if (t1_val < 0) t1_val = getJsonInt(body, "t0_step_off", -1);
    if (t1_val >= 20 && t1_val <= 5000) config.t1 = t1_val;

    // t2: Thời gian sáng sau khi rời cầu thang
    int t2_val = getJsonInt(body, "t2", -1);
    if (t2_val < 0) t2_val = getJsonInt(body, "t1_hold", -1);
    if (t2_val >= 500 && t2_val <= 60000) config.t2 = t2_val;

    int rb = getJsonInt(body, "reboot", 1);
    if (rb == 1) shouldReboot = true;

    eeprom_save();
    server.sendHeader(F("Connection"), F("close"));
    server.send_P(200, PSTR("application/json"), PSTR("{\"success\":true,\"reboot\":true}"));
    if (shouldReboot) {
      // Hẹn giờ khởi động lại an toàn trong loop, không block ở đây
      should_restart = true;
      restart_start_time = millis();
    }
    return;
  }

  // Fallback URL-encoded
  if (server.hasArg("mode")) {
    int m = server.arg("mode").toInt();
    if (m >= 1 && m <= 9) config.mode = m;
  }
  if (server.hasArg("number_step")) {
    int s = server.arg("number_step").toInt();
    if (s >= 1 && s <= MAX_STEPS) config.number_step = s;
  }
  if (server.hasArg("t0")) config.t0 = server.arg("t0").toInt();
  else if (server.hasArg("t0_step_on")) config.t0 = server.arg("t0_step_on").toInt();

  if (server.hasArg("t1")) config.t1 = server.arg("t1").toInt();
  else if (server.hasArg("t0_step_off")) config.t1 = server.arg("t0_step_off").toInt();

  if (server.hasArg("t2")) config.t2 = server.arg("t2").toInt();
  else if (server.hasArg("t1_hold")) config.t2 = server.arg("t1_hold").toInt();

  if (server.hasArg("reboot")) shouldReboot = (server.arg("reboot").toInt() == 1);
  else shouldReboot = true;

  eeprom_save();
  server.sendHeader(F("Connection"), F("close"));
  server.send_P(200, PSTR("application/json"), PSTR("{\"success\":true,\"reboot\":true}"));
  if (shouldReboot) {
    should_restart = true;
    restart_start_time = millis();
  }
}

void handleApiTrigger() {
  if (server.hasArg("plain")) {
    const char* body = server.arg("plain").c_str();
    char action[16];
    if (getJsonStr(body, "action", action, sizeof(action))) {
      if (strcmp(action, "up") == 0) {
        event_ss1_trigger = true;
      } else if (strcmp(action, "down") == 0) {
        event_ss2_trigger = true;
      } else if (strcmp(action, "both") == 0) {
        event_ss1_trigger = true;
        event_ss2_trigger = true;
      } else if (strcmp(action, "off") == 0) {
        stair_state = STATE_IDLE;
        fade_step = 0;
        shift595_set_all(false);
      }
    }
  }
  server.sendHeader(F("Connection"), F("close"));
  server.send_P(200, PSTR("application/json"), PSTR("{\"success\":true}"));
}

void handleApiReboot() {
  server.sendHeader(F("Connection"), F("close"));
  server.send_P(200, PSTR("application/json"), PSTR("{\"success\":true}"));
  should_restart = true;
  restart_start_time = millis();
}

void setupWebServer() {
  // Bỏ qua favicon để tiết kiệm bộ nhớ và tránh nghẽn mạng
  server.on(F("/favicon.ico"), HTTP_GET, []() {
    server.send(204);
  });

  // Trang chủ & các đường dẫn alias
  server.on(F("/"), HTTP_GET, handleRoot);
  server.on(F("/index.html"), HTTP_GET, handleRoot);
  server.on(F("/config"), HTTP_GET, handleRoot);
  server.on(F("/Config"), HTTP_GET, handleRoot);
  server.on(F("/login"), HTTP_GET, handleRoot);

  // Web API
  server.on(F("/api/config"), HTTP_GET, handleApiGetConfig);
  server.on(F("/api/config"), HTTP_POST, handleApiPostConfig);
  server.on(F("/api/trigger"), HTTP_POST, handleApiTrigger);
  server.on(F("/api/reboot"), HTTP_POST, handleApiReboot);

  // Tương thích ngược đường dẫn cũ
  server.on(F("/sendweb"), HTTP_GET, handleApiGetConfig);
  server.on(F("/Config"), HTTP_POST, handleApiPostConfig);

  // Tự động mở webpage hoặc chuyển hướng Captive Portal cho tất cả request không khớp
  server.onNotFound(handleNotFound);
}
