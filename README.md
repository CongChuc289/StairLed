# HỆ THỐNG ĐIỀU KHIỂN LED CẦU THANG THÔNG MINH 32 BẬC (SMART STAIR LED CONTROLLER)

Hệ thống điều khiển LED cầu thang thông minh sử dụng vi điều khiển **ESP8266** (NodeMCU / WeMos D1) kết hợp với **4 IC ghi dịch 74HC595 mắc nối tiếp (daisy-chain)** để điều khiển độc lập tối đa **32 bậc cầu thang**, kết hợp **2 cảm biến tiệm cận ở 2 đầu cầu thang**, **giao diện Web cấu hình thông số qua WiFi** và **trình mô phỏng trực quan độc lập (Simulator)**.

---

## 🌟 TÍNH NĂNG CHÍNH

1. **Điều khiển tối đa 32 bậc LED độc lập:**
   - 4 IC 74HC595 mắc nối tiếp (Daisy-Chain 32 bit).
   - Cố định phần cứng: Mức 1 (HIGH) sáng LED.
   - Số bậc hoạt động $N$ có thể điều chỉnh linh hoạt từ **1 đến 32 bậc** tùy theo công trình thực tế (mặc định 25 bậc).
   - Tối ưu chuẩn vi xung `delayMicroseconds` cho Clock và Latch, chống méo/trượt xung khi dịch bit trên tải dung cao của 4 IC.

2. **Cảm biến tiệm cận 2 đầu cầu thang:**
   - Mức tín hiệu phần cứng: **Tín hiệu 1 (HIGH) khi phát hiện người**, **0 (LOW) khi không có người**.
   - **SS1:** Cảm biến dưới (bậc 1).
   - **SS2:** Cảm biến trên (bậc $N$).
   - Tích hợp bộ lọc chống dội tín hiệu phần mềm (**Debounce 30ms**) lọc sạch 100% nhiễu xung kim.

3. **Chuẩn 3 mốc thời gian độc lập ($t_0, t_1, t_2$):**
   - **$t_0$ (Thời gian sáng từng bậc):** Tốc độ sáng dồn từng bậc khi có người bước vào (20ms – 1000ms).
   - **$t_1$ (Thời gian tắt từng bậc):** Tốc độ tắt dồn/tắt lần lượt từng bậc (20ms – 1000ms).
   - **$t_2$ (Thời gian giữ sáng sau khi rời cầu thang):** Thời gian giữ sáng toàn bộ $N$ bậc sau khi người đi hết cầu thang (0.5s – 30s) trước khi bắt đầu chu trình tắt lần lượt.

4. **Xử lý toàn diện 6 kịch bản thông minh (Non-blocking 100% bằng FSM & `millis()`):**
   - **Trường hợp 1 (Đi từ dưới lên):**
     - SS1 kích hoạt $\rightarrow$ Bật sáng dồn từng bậc từ 1 đến $N$ với tốc độ $t_0$ (không cần xét cảm biến lối ra).
     - Khi sáng đủ $N$ bậc $\rightarrow$ Tự động chuyển sang **GIỮ SÁNG toàn bộ trong thời gian $t_2$ (HOLD)**.
     - Sau đúng thời gian $t_2$ $\rightarrow$ Bắt đầu **TẮT LẦN LƯỢT TỪ DƯỚI LÊN** ($1 \rightarrow N$, mỗi bậc tắt sau thời gian $t_1$) $\rightarrow$ Trở về trạng thái CHỜ (IDLE).
   - **Trường hợp 2 (Đi từ trên xuống):**
     - SS2 kích hoạt $\rightarrow$ Bật sáng dồn từng bậc từ $N$ về 1 với tốc độ $t_0$ (không cần xét cảm biến lối ra).
     - Khi sáng đủ $N$ bậc $\rightarrow$ Tự động chuyển sang **GIỮ SÁNG toàn bộ trong thời gian $t_2$ (HOLD)**.
     - Sau đúng thời gian $t_2$ $\rightarrow$ Bắt đầu **TẮT LẦN LƯỢT TỪ TRÊN XUỐNG** ($N \rightarrow 1$, mỗi bậc tắt sau thời gian $t_1$) $\rightarrow$ Trở về trạng thái CHỜ (IDLE).
   - **Trường hợp 3 (Hai người đi ngược chiều cùng lúc):**
     - Cả SS1 và SS2 kích hoạt $\rightarrow$ Sáng dồn đồng thời từ 2 phía vào bậc tâm với tốc độ $t_0$.
     - Hai luồng sáng chạm nhau ở trung tâm $\rightarrow$ Chuyển sang **GIỮ SÁNG toàn bộ trong thời gian $t_2$ (HOLD)**.
     - Hết $t_2$ $\rightarrow$ Bắt đầu **TẮT LẦN LƯỢT TỪ 2 ĐẦU VÀO GIỮA** (mỗi bậc tắt cách nhau thời gian $t_1$) $\rightarrow$ Trở về trạng thái CHỜ (IDLE).
   - **Trường hợp 4 (Đang trong quá trình tắt dần từ dưới lên):**
     - Người mới đi lên từ dưới: HỦY tắt ngay lập tức, bật sáng lại toàn bộ các bậc và gia hạn lại thời gian $t_2$.
     - Người mới đi xuống từ trên: HỦY tắt ngay lập tức, bật sáng toàn bộ và chuyển hướng kết thúc cho người đi xuống.
     - Cả hai cảm biến cùng kích hoạt: HỦY tắt, giữ toàn bộ LED sáng và gia hạn $t_2$.
   - **Trường hợp 5 (Đang trong quá trình tắt dần từ trên xuống):**
     - Người mới đi lên từ dưới: HỦY tắt, bật sáng lại toàn bộ và chuyển hướng kết thúc cho người đi lên.
     - Người mới đi xuống từ trên: HỦY tắt, bật sáng lại toàn bộ và gia hạn thời gian $t_2$.
     - Cả hai cảm biến cùng kích hoạt: HỦY tắt, giữ toàn bộ LED sáng liên tục.
   - **Trường hợp 6 (Đang trong quá trình tắt dần 2 đầu vào giữa):**
     - Bất kỳ cảm biến nào kích hoạt: HỦY tắt toàn bộ ngay lập tức, bật sáng lại toàn bộ và tiếp tục giữ sáng $t_2$.
   - **Gia hạn an toàn (Hold Safety Retrigger):** Khi đèn đang ở giai đoạn giữ sáng ($t_2$), nếu tiếp tục có người bước vào, hệ thống tự động reset lại thời gian $t_2$ để đèn không bao giờ tắt khi vẫn còn người trên cầu thang.

5. **Giao diện Web cấu hình thông minh qua WiFi & Captive Portal:**
   - **Tự động mở trang cấu hình (Captive Portal):** Tự động chuyển hướng và mở ngay trang cài đặt khi điện thoại/máy tính kết nối vào mạng WiFi hoặc khi nhập địa chỉ IP bất kỳ.
   - **Nút gộp "Lưu cấu hình & Khởi động lại":** Lưu tất cả thông số vào Flash EEPROM và tự động khởi động lại vi điều khiển trong 1 thao tác duy nhất, có modal đếm ngược trực quan.
   - **Thử nghiệm kịch bản (Test):** Các nút bấm mô phỏng [⬆ Đi từ dưới lên], [⬇ Đi từ trên xuống], [↕ Cả 2 cùng đi], [⏹ Tắt hết].
   - **Cài đặt thông số:** Chế độ chạy, số bậc $N$, thời gian sáng $t_0$, thời gian tắt $t_1$, thời gian giữ sáng $t_2$.

6. **Trình mô phỏng trực quan độc lập (Standalone Web Simulator):**
   - File [simulator.html](file:///d:/Project/StairLed-main/StairLed-main/Software/StairLed/simulator.html) chạy 100% offline trên trình duyệt.
   - Giàn LED cầu thang hiển thị hiệu ứng phát sáng ấm áp theo thời gian thực.
   - Tích hợp bảng trạng thái FSM, thanh đếm ngược $t_2$ và khung nhật ký Serial Monitor.

---

## 🧩 PHÂN CHIA 4 IC 74HC595 & BẬC CẦU THANG (32 BẬC)

Chuỗi 4 IC 74HC595 được mắc nối tiếp (Daisy-Chain) nhận dữ liệu từ chân **D7 (DATA)** của ESP8266:
$$\text{ESP8266 (D7)} \xrightarrow{\text{DATA}} \mathbf{IC1} \xrightarrow{\text{QH'}} \mathbf{IC2} \xrightarrow{\text{QH'}} \mathbf{IC3} \xrightarrow{\text{QH'}} \mathbf{IC4}$$

| IC Ghi Dịch | Bậc Cầu Thang Phụ Trách | BitMask (32-bit) | Ngõ Ra Phần Cứng | Linh Kiện Công Suất |
| :---: | :---: | :---: | :---: | :---: |
| **IC1 (U1)** | **Bậc 1 đến Bậc 8** | Bit 0 đến Bit 7 | `O0_HC` $\dots$ `O7_HC` | 8x MOSFET N-CH SQD50N03 |
| **IC2 (U2)** | **Bậc 9 đến Bậc 16** | Bit 8 đến Bit 15 | `O8_HC` $\dots$ `O15_HC` | 8x MOSFET N-CH SQD50N03 |
| **IC3 (U3)** | **Bậc 17 đến Bậc 24** | Bit 16 đến Bit 23 | `O16_HC` $\dots$ `O23_HC` | 8x MOSFET N-CH SQD50N03 |
| **IC4 (U4)** | **Bậc 25 đến Bậc 32** | Bit 24 đến Bit 31 | `O24_HC` $\dots$ `O31_HC` | 8x MOSFET N-CH SQD50N03 |

* **Quy ước mức logic:** Mức **1 (HIGH)** mở MOSFET kéo chân âm LED dải xuống GND $\rightarrow$ **Sáng LED**. Mức **0 (LOW)** $\rightarrow$ **Tắt LED**.
* **Thứ tự dịch bit:** Dịch từ bit 31 (MSB) về bit 0 (LSB). Bit 31 vào đầu tiên sẽ chạy xuyên suốt 32 xung Clock để dừng lại ở ngõ ra cuối cùng của **IC4 (Bậc 32)**; Bit 0 vào cuối cùng sẽ dừng lại ở ngõ ra đầu tiên của **IC1 (Bậc 1)**.

---

## 🔌 SƠ ĐỒ KẾT NỐI PHẦN CỨNG (PINOUT)

| Chân ESP8266 (NodeMCU) | Tên chân trên mạch | Chức năng kết nối |
| :--- | :--- | :--- |
| **D3** (GPIO0) | **LAT / ST_CP** | Chốt dữ liệu 74HC595 (Latch Clock) |
| **D4** (GPIO2) | **CLK / SH_CP** | Xung nhịp dịch dữ liệu 74HC595 (Shift Clock) |
| **D7** (GPIO13)| **DATA / DS** | Đường truyền dữ liệu nối tiếp 74HC595 (Serial Data) |
| **D2** (GPIO4) | **SS1** | Cảm biến tiệm cận **DƯỚI** cầu thang (mức HIGH khi có người) |
| **D1** (GPIO5) | **SS2** | Cảm biến tiệm cận **TRÊN** cầu thang (mức HIGH khi có người) |
| **D5** (GPIO14)| **BT1** | Nút nhấn vật lý 1 trên mạch (Test đi lên) |
| **D6** (GPIO12)| **BT2** | Nút nhấn vật lý 2 trên mạch (Test đi xuống) |

---

## 📱 KẾT NỐI WIFI VÀ CẤU HÌNH

1. Cấp nguồn 5V cho mạch.
2. Dùng điện thoại hoặc máy tính kết nối vào mạng WiFi do ESP8266 phát ra:
   - **Tên WiFi (SSID):** `StairLed_CTECK`
   - **Mật khẩu (Password):** `12345678`
3. Màn hình đăng nhập sẽ tự động bật lên (Captive Portal), hoặc mở trình duyệt truy cập:
   - **Địa chỉ IP:** `http://192.168.4.1`
4. Điều chỉnh các thông số ($N, t_0, t_1, t_2$), chọn chế độ hoạt động và nhấn **"💾 LƯU CẤU HÌNH & KHỞI ĐỘNG LẠI"**.
