#ifndef _WEBPAGE_H
#define _WEBPAGE_H

#include <Arduino.h>

const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <title>Cài Đặt LED Cầu Thang</title>
  <style>
    :root {
      --primary: #2563eb;
      --success: #10b981;
      --warning: #f59e0b;
      --danger: #ef4444;
      --bg: #0f172a;
      --card-bg: #1e293b;
      --text: #f8fafc;
      --text-muted: #94a3b8;
      --border: #334155;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background: var(--bg);
      color: var(--text);
      line-height: 1.5;
      padding: 16px;
      min-height: 100vh;
    }
    .container { max-width: 560px; margin: 0 auto; }
    header {
      text-align: center;
      margin-bottom: 20px;
      padding-bottom: 12px;
      border-bottom: 1px solid var(--border);
    }
    header h1 { font-size: 20px; color: #38bdf8; margin-bottom: 4px; }
    header p { font-size: 13px; color: var(--text-muted); }
    .ip-badge {
      display: inline-block;
      margin-top: 6px;
      background: #0f172a;
      border: 1px solid var(--border);
      border-radius: 20px;
      padding: 3px 12px;
      font-size: 12px;
      color: #38bdf8;
      font-family: monospace;
    }

    .card {
      background: var(--card-bg);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 18px;
      margin-bottom: 18px;
      box-shadow: 0 4px 6px -1px rgba(0,0,0,0.25);
    }
    .card-title {
      font-size: 15px;
      font-weight: 600;
      color: #e2e8f0;
      margin-bottom: 14px;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }

    /* Nhóm nút Test */
    .btn-group {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
    }
    button, .btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
      padding: 12px 14px;
      border-radius: 8px;
      border: none;
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.15s ease;
      color: white;
      background: #334155;
      text-align: center;
    }
    button:hover { filter: brightness(1.15); }
    button:active { transform: scale(0.98); }
    .btn-primary { background: var(--primary); }
    .btn-warning { background: var(--warning); color: #0f172a; }
    .btn-danger { background: var(--danger); }
    .btn-success { background: linear-gradient(135deg, #10b981 0%, #059669 100%); }

    /* Form cài đặt */
    .form-group {
      margin-bottom: 16px;
    }
    .form-label {
      display: flex;
      justify-content: space-between;
      font-size: 13px;
      font-weight: 500;
      margin-bottom: 6px;
      color: #cbd5e1;
    }
    .form-val {
      color: #38bdf8;
      font-weight: 600;
      font-family: monospace;
    }
    input[type="range"] {
      width: 100%;
      height: 6px;
      background: #334155;
      border-radius: 3px;
      outline: none;
      -webkit-appearance: none;
    }
    input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 22px;
      height: 22px;
      border-radius: 50%;
      background: #38bdf8;
      cursor: pointer;
      box-shadow: 0 0 6px rgba(56, 189, 248, 0.5);
    }
    select {
      width: 100%;
      padding: 10px 12px;
      background: #0f172a;
      border: 1px solid var(--border);
      border-radius: 8px;
      color: white;
      font-size: 14px;
      outline: none;
    }
    select:focus { border-color: #38bdf8; }

    .toast {
      position: fixed;
      bottom: 20px;
      left: 50%;
      transform: translateX(-50%) translateY(100px);
      background: rgba(16, 185, 129, 0.95);
      color: white;
      padding: 10px 20px;
      border-radius: 8px;
      font-size: 14px;
      font-weight: 500;
      box-shadow: 0 4px 12px rgba(0,0,0,0.3);
      transition: transform 0.3s ease;
      z-index: 1000;
      pointer-events: none;
    }
    .toast.show { transform: translateX(-50%) translateY(0); }
    .toast.error { background: var(--danger); }

    /* Modal đếm ngược khởi động lại */
    .reboot-overlay {
      display: none;
      position: fixed;
      top: 0; left: 0; right: 0; bottom: 0;
      background: rgba(15, 23, 42, 0.92);
      backdrop-filter: blur(5px);
      z-index: 2000;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .reboot-overlay.active {
      display: flex;
    }
    .reboot-box {
      background: var(--card-bg);
      border: 1px solid var(--border);
      border-radius: 16px;
      padding: 28px 24px;
      text-align: center;
      max-width: 380px;
      width: 100%;
      box-shadow: 0 10px 25px rgba(0, 0, 0, 0.5);
    }
    .spinner {
      width: 48px;
      height: 48px;
      border: 4px solid rgba(56, 189, 248, 0.2);
      border-top-color: #38bdf8;
      border-radius: 50%;
      animation: spin 1s linear infinite;
      margin: 0 auto 16px;
    }
    @keyframes spin {
      to { transform: rotate(360deg); }
    }
    .reboot-box h3 {
      font-size: 17px;
      color: #38bdf8;
      margin-bottom: 8px;
    }
    .reboot-box p {
      font-size: 13px;
      color: var(--text-muted);
      margin-bottom: 18px;
      line-height: 1.5;
    }
    .countdown-circle {
      display: inline-block;
      font-size: 24px;
      font-weight: 700;
      color: #10b981;
      background: #0f172a;
      border: 2px solid #10b981;
      width: 56px;
      height: 56px;
      line-height: 52px;
      border-radius: 50%;
      margin: 0 auto;
    }

    footer {
      text-align: center;
      font-size: 12px;
      color: var(--text-muted);
      margin-top: 20px;
      padding-bottom: 12px;
      line-height: 1.6;
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>LED CẦU THANG THÔNG MINH</h1>
    </header>

    <!-- Thẻ 1: Nút Test mô phỏng các kịch bản -->
    <div class="card">
      <div class="card-title">🧪 Thử nghiệm kịch bản (Test)</div>
      <div class="btn-group">
        <button class="btn btn-primary" onclick="triggerTest('up')">⬆ Đi từ dưới lên</button>
        <button class="btn btn-primary" onclick="triggerTest('down')">⬇ Đi từ trên xuống</button>
        <button class="btn btn-warning" onclick="triggerTest('both')">↕ Cả 2 cùng đi</button>
        <button class="btn btn-danger" onclick="triggerTest('off')">⏹ Tắt hết</button>
      </div>
    </div>

    <!-- Thẻ 2: Cấu hình thông số -->
    <div class="card">
      <div class="card-title">⚙ Cài đặt thông số hoạt động</div>

      <form id="config-form" onsubmit="saveAndReboot(event)">
        <!-- Chế độ hoạt động -->
        <div class="form-group">
          <label class="form-label">Chế độ hoạt động</label>
          <select id="mode" name="mode">
            <option value="1">1. Cảm biến thông minh</option>
            <option value="2">2. Kiểm tra LED (Sáng tất cả các bậc)</option>
            <option value="3">3. Sáng dồn từ dưới lên</option>
            <option value="4">4. Sáng dồn từ trên xuống</option>
            <option value="5">5. Sao Băng / Rượt đuổi</option>
            <option value="6">6. Chớp nháy đồng bộ</option>
            <option value="7">7. Sáng, tắt so le chẵn lẻ</option>
            <option value="8">8. Tắt toàn bộ LED</option>
          </select>
        </div>

        <!-- Số bậc cầu thang -->
        <div class="form-group">
          <div class="form-label">
            <span>Số bậc cầu thang:</span>
            <span class="form-val" id="val-number_step">25</span>
          </div>
          <input type="range" id="number_step" name="number_step" min="1" max="32" value="25" oninput="updateRangeVal('number_step', '')">
        </div>

        <!-- Thời gian sáng từng bậc t0 -->
        <div class="form-group">
          <div class="form-label">
            <span>Thời gian sáng từng bậc (t0):</span>
            <span class="form-val" id="val-t0">150 ms</span>
          </div>
          <input type="range" id="t0" name="t0" min="20" max="1000" step="10" value="150" oninput="updateRangeVal('t0', ' ms')">
        </div>

        <!-- Thời gian tắt từng bậc t1 -->
        <div class="form-group">
          <div class="form-label">
            <span>Thời gian tắt từng bậc (t1):</span>
            <span class="form-val" id="val-t1">150 ms</span>
          </div>
          <input type="range" id="t1" name="t1" min="20" max="1000" step="10" value="150" oninput="updateRangeVal('t1', ' ms')">
        </div>

        <!-- Thời gian sáng sau khi rời cầu thang t2 -->
        <div class="form-group">
          <div class="form-label">
            <span>Thời gian sáng sau khi rời cầu thang (t2):</span>
            <span class="form-val" id="val-t2">5.0 s</span>
          </div>
          <input type="range" id="t2" name="t2" min="500" max="30000" step="500" value="5000" oninput="updateHoldVal()">
        </div>

        <!-- Nút gộp: Lưu cấu hình & Khởi động lại -->
        <div style="margin-top: 22px;">
          <button type="submit" id="btn-save-reboot" class="btn btn-success" style="width: 100%; padding: 14px; font-size: 14px; font-weight: 700; letter-spacing: 0.5px;">
            💾 LƯU CẤU HÌNH &amp; KHỞI ĐỘNG LẠI
          </button>
        </div>
      </form>
    </div>

    <footer>
      <div>Hệ thống điều khiển LED cầu thang make by congchuc289@gmail.com</div>
    </footer>
  </div>

  <!-- Toast thông báo nhanh -->
  <div id="toast" class="toast">Cấu hình thành công!</div>

  <!-- Modal đếm ngược khi lưu cấu hình và khởi động lại -->
  <div id="reboot-modal" class="reboot-overlay">
    <div class="reboot-box">
      <div class="spinner"></div>
      <h3>Đang lưu &amp; Khởi động lại</h3>
      <p>Cấu hình đã được lưu thành công!<br>Thiết bị đang khởi động lại để áp dụng cài đặt mới.</p>
      <div class="countdown-circle" id="countdown-val">6</div>
    </div>
  </div>

  <script>
    function showToast(msg, isError = false) {
      const t = document.getElementById('toast');
      t.innerText = msg;
      t.className = 'toast show' + (isError ? ' error' : '');
      setTimeout(() => { t.className = 'toast'; }, 2500);
    }

    function updateRangeVal(id, suffix) {
      document.getElementById('val-' + id).innerText = document.getElementById(id).value + suffix;
    }

    function updateHoldVal() {
      const ms = parseInt(document.getElementById('t2').value);
      document.getElementById('val-t2').innerText = (ms / 1000).toFixed(1) + ' s';
    }

    // Lấy cấu hình từ ESP
    function loadConfig() {
      fetch('/api/config')
        .then(r => r.json())
        .then(cfg => {
          document.getElementById('mode').value = cfg.mode;
          document.getElementById('number_step').value = cfg.number_step;
          document.getElementById('t0').value = cfg.t0 || cfg.t0_step_on || 150;
          document.getElementById('t1').value = cfg.t1 || cfg.t0_step_off || 150;
          document.getElementById('t2').value = cfg.t2 || cfg.t1_hold || 5000;

          updateRangeVal('number_step', '');
          updateRangeVal('t0', ' ms');
          updateRangeVal('t1', ' ms');
          updateHoldVal();
        })
        .catch(err => console.error("Lỗi tải config:", err));
    }

    // Đếm ngược và tự động tải lại trang
    function startRebootCountdown(seconds) {
      const modal = document.getElementById('reboot-modal');
      const countEl = document.getElementById('countdown-val');
      modal.classList.add('active');
      let remain = seconds;
      countEl.innerText = remain;

      const timer = setInterval(() => {
        remain--;
        if (remain > 0) {
          countEl.innerText = remain;
        } else {
          clearInterval(timer);
          countEl.innerText = "...";
          window.location.reload();
        }
      }, 1000);
    }

    // Lưu cấu hình và khởi động lại cùng ở 1 nút
    function saveAndReboot(e) {
      e.preventDefault();
      const saveBtn = document.getElementById('btn-save-reboot');
      saveBtn.disabled = true;
      saveBtn.innerText = "⏳ Đang lưu & khởi động lại...";

      const data = {
        mode: parseInt(document.getElementById('mode').value),
        number_step: parseInt(document.getElementById('number_step').value),
        t0: parseInt(document.getElementById('t0').value),
        t1: parseInt(document.getElementById('t1').value),
        t2: parseInt(document.getElementById('t2').value),
        reboot: 1
      };

      fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
      })
      .then(r => r.json())
      .then(res => {
        if (res.success) {
          startRebootCountdown(6);
        } else {
          showToast("Lỗi khi lưu cấu hình!", true);
          saveBtn.disabled = false;
          saveBtn.innerText = "💾 LƯU CẤU HÌNH & KHỞI ĐỘNG LẠI";
        }
      })
      .catch(err => {
        // Nếu ESP khởi động lại ngay làm ngắt kết nối fetch, vẫn kích hoạt đếm ngược
        startRebootCountdown(6);
      });
    }

    // Nút Test kịch bản
    function triggerTest(action) {
      fetch('/api/trigger', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: action })
      }).then(r => r.json()).then(res => {
        showToast("Đã kích hoạt lệnh test: " + action);
      }).catch(err => {});
    }

    // Bắt đầu nạp cấu hình khi mở trang
    loadConfig();
  </script>
</body>
</html>
)rawliteral";

#endif // _WEBPAGE_H
