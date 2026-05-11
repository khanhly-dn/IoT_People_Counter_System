// ============================================================
// IoT People Counter — CẢI TIẾN AIoT
// Thêm: gửi telemetry lên FastAPI backend qua HTTP POST
// Giữ nguyên: IR sensor, Servo, OLED, Buzzer, LED, WebServer
// ============================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ---- CẤU HÌNH WiFi ----
#define WIFI_SSID   "Khanh"
#define WIFI_PASS   "@133057k"

// ---- BACKEND SERVER (IP máy tính chạy FastAPI) ----
// Mở cmd → ipconfig → lấy IPv4 Address
#define BACKEND_URL "http://192.168.100.215:8000/api/telemetry"
#define CMD_URL     "http://192.168.100.215:8000/api/commands/people_counter_zone_A/pending"
#define CMD_DONE    "http://192.168.100.215:8000/api/commands/"
#define DEVICE_ID    "people_counter_zone_A"
#define ZONE_ID      "zone_A_entrance"

// ---- CHÂN KẾT NỐI ----
#define PIN_IR       34
#define PIN_LED       2
#define PIN_BUZZER    5
#define PIN_SERVO    18

// ---- OLED ----
#define OLED_WIDTH  128
#define OLED_HEIGHT  64
#define OLED_ADDR  0x3C
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// ---- SERVO ----
#define SERVO_MO     90
#define SERVO_DONG    0
#define SERVO_CHAN   45
#define THOI_GIAN_MO 3000

Servo servo;
unsigned long tgMoCua   = 0;
bool dangMoCua          = false;

// ---- BIẾN ĐẾM NGƯỜI ----
int soNguoiVao      = 0;
int soNguoiRa       = 0;
int soNguoiHienTai  = 0;
int GIOI_HAN_PHONG  = 10;

bool canhBaoQuaTai   = false;
bool cheDoChangBat   = true;
bool ghiDeQuaTai     = false;
bool trangThaiIR_truoc = HIGH;

// ---- TIMING ----
unsigned long tgBatDau       = 0;
unsigned long tgCapNhatOLED  = 0;
unsigned long tgGuiBackend   = 0;
unsigned long tgGuiInterval  = 10000; // Gửi backend mỗi 10 giây
unsigned long tgNhanLenh     = 0;     // Nhận lệnh mỗi 3 giây
bool coSuKienMoi             = false;

float batteryLevel = 95.0;

WebServer server(80);

// ============================================================
// GỬI TELEMETRY LÊN BACKEND
// ============================================================
void guiTelemetry(String lyDo) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] Không có WiFi, bỏ qua.");
    return;
  }

  int phanTram = (GIOI_HAN_PHONG > 0)
                 ? min(soNguoiHienTai * 100 / GIOI_HAN_PHONG, 100)
                 : 0;

  StaticJsonDocument<512> doc;
  doc["device_id"]        = DEVICE_ID;
  doc["zone_id"]          = ZONE_ID;
  doc["so_nguoi_hien_tai"] = soNguoiHienTai;
  doc["so_nguoi_vao"]     = soNguoiVao;
  doc["so_nguoi_ra"]      = soNguoiRa;
  doc["gioi_han_phong"]   = GIOI_HAN_PHONG;
  doc["phan_tram_su_dung"] = phanTram;
  doc["canh_bao_qua_tai"] = canhBaoQuaTai;
  doc["che_do_chan"]       = cheDoChangBat;
  doc["cua_dang_mo"]      = dangMoCua;
  doc["battery_level"]    = batteryLevel;
  doc["network_signal"]   = WiFi.RSSI();
  doc["ly_do_gui"]        = lyDo;

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  http.begin(BACKEND_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(3000);

  int httpCode = http.POST(payload);

  if (httpCode == 200 || httpCode == 201) {
    Serial.println("[HTTP] ✓ Gửi OK → " + lyDo +
                   " | Người: " + String(soNguoiHienTai) +
                   " | " + String(phanTram) + "%");
    String response = http.getString();
    Serial.println("[HTTP] AI: " + response.substring(0, 100));
  } else {
    Serial.println("[HTTP] Lỗi: " + String(httpCode));
  }
  http.end();
  coSuKienMoi = false;
}

void nhanLenh() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(CMD_URL);
  http.setTimeout(3000);

  int httpCode = http.GET();
  if (httpCode == 200) {
    String response = http.getString();
    StaticJsonDocument<512> doc;
    deserializeJson(doc, response);

    String lenh = doc["command"] | "";
    int cmdId   = doc["id"]      | 0;

    if (lenh == "" || cmdId == 0) {
      http.end();
      return; // Không có lệnh mới
    }

    Serial.println("[CMD] Nhận lệnh: " + lenh + " (ID:" + String(cmdId) + ")");

    if (lenh == "BLOCK_ENTRY") {
      cheDoChangBat = true;
   
      if (soNguoiHienTai >= GIOI_HAN_PHONG) {
        canhBaoQuaTai = true;
        servo.write(SERVO_CHAN);
        beepCanhBao();
      }
      Serial.println("[CMD] → Chế độ chặn BẬT!");
    }
    else if (lenh == "ALLOW_ENTRY") {
      cheDoChangBat = false;
      canhBaoQuaTai = false;
      servo.write(SERVO_DONG);
      beepVao();
      Serial.println("[CMD] → Đã mở cửa!");
    }
    else if (lenh == "RESET_COUNT") {
      soNguoiVao = soNguoiRa = soNguoiHienTai = 0;
      canhBaoQuaTai = false;
      ghiDeQuaTai   = false;
      servo.write(SERVO_DONG);
      Serial.println("[CMD] → Đã reset đếm!");
      veOLED();
    }
    else if (lenh == "SET_LIMIT") {
      int val = doc["payload"] | 10;
      GIOI_HAN_PHONG = val;
      canhBaoQuaTai  = (soNguoiHienTai >= GIOI_HAN_PHONG);
      Serial.println("[CMD] → Giới hạn mới: " + String(val));
    }
    else if (lenh == "TEST_BUZZER") {
      beepVao();
      Serial.println("[CMD] → Buzzer test!");
    }

    // Báo server đã thực thi xong
    HTTPClient http2;
    http2.begin(CMD_DONE + String(cmdId) + "/done");
    http2.addHeader("Content-Type", "application/json");
    http2.POST("");
    http2.end();

    guiTelemetry("cmd_executed");
    veOLED();
  }
  http.end();
}

void moCua() {
  if (canhBaoQuaTai && cheDoChangBat && !ghiDeQuaTai) {
    servo.write(SERVO_CHAN);
    Serial.println("[SERVO] Chặn cửa — phòng quá tải!");
    return;
  }
  servo.write(SERVO_MO);
  dangMoCua = true;
  tgMoCua   = millis();
  Serial.println("[SERVO] Mở cửa");
}

void dongCua() {
  servo.write(SERVO_DONG);
  dangMoCua   = false;
  ghiDeQuaTai = false;
  Serial.println("[SERVO] Đóng cửa");
}

void capNhatServo() {
  if (dangMoCua && millis() - tgMoCua >= THOI_GIAN_MO) dongCua();
  if (canhBaoQuaTai && cheDoChangBat && !dangMoCua && !ghiDeQuaTai)
    servo.write(SERVO_CHAN);
  if (!canhBaoQuaTai && !dangMoCua)
    servo.write(SERVO_DONG);
}

void beepVao() {
  digitalWrite(PIN_BUZZER, HIGH); delay(80);
  digitalWrite(PIN_BUZZER, LOW);  delay(80);
  digitalWrite(PIN_BUZZER, HIGH); delay(80);
  digitalWrite(PIN_BUZZER, LOW);
}
void beepRa() {
  digitalWrite(PIN_BUZZER, HIGH); delay(300);
  digitalWrite(PIN_BUZZER, LOW);
}
void beepCanhBao() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(PIN_BUZZER, HIGH); delay(50);
    digitalWrite(PIN_BUZZER, LOW);  delay(50);
  }
}

// ============================================================
// LED
// ============================================================
void capNhatLED() {
  if (canhBaoQuaTai)
    digitalWrite(PIN_LED, (millis() / 300) % 2);
  else if (soNguoiHienTai > 0)
    digitalWrite(PIN_LED, HIGH);
  else
    digitalWrite(PIN_LED, LOW);
}

// ============================================================
// OLED
// ============================================================
void veOLED() {
  oled.clearDisplay();
  oled.fillRect(0, 0, 128, 14, WHITE);
  oled.setTextColor(BLACK);
  oled.setTextSize(1);
  oled.setCursor(10, 3);
  oled.print("PEOPLE COUNTER AIoT");
  oled.setTextColor(WHITE);

  if (canhBaoQuaTai && cheDoChangBat) {
    oled.fillRect(0, 0, 128, 64, WHITE);
    oled.setTextColor(BLACK);
    oled.setTextSize(1);
    oled.setCursor(22, 5);  oled.print("!! CANH BAO !!");
    oled.setTextSize(2);
    oled.setCursor(10, 18); oled.print("QUA TAI!");
    oled.setTextSize(1);
    oled.setCursor(8, 42);  oled.print("Cua bi chan!");
    oled.setCursor(8, 54);  oled.print("Max:"); oled.print(GIOI_HAN_PHONG);
    oled.setTextColor(WHITE);
    oled.display();
    return;
  }

  oled.setTextSize(3);
  String soChu = String(soNguoiHienTai);
  int x = (128 - soChu.length() * 18) / 2;
  oled.setCursor(x, 17);
  oled.print(soChu);

  oled.setTextSize(1);
  oled.setCursor(20, 44);
  oled.print("TRONG PHONG");

  oled.drawLine(0, 53, 128, 53, WHITE);
  oled.setCursor(0, 56);
  oled.print("Chan:"); oled.print(cheDoChangBat ? "BAT" : "TAT");
  oled.setCursor(65, 56);
  oled.print("V:"); oled.print(soNguoiVao);
  oled.print(" R:"); oled.print(soNguoiRa);

  oled.display();
}

// ============================================================
// WEB DASHBOARD
// ============================================================
String trangChu() {
  unsigned long uptime = (millis() - tgBatDau) / 1000;
  int phanTram = GIOI_HAN_PHONG > 0
                 ? min(soNguoiHienTai * 100 / GIOI_HAN_PHONG, 100) : 0;

  String mauBar = phanTram >= 100 ? "#e74c3c" : phanTram >= 70 ? "#f39c12" : "#27ae60";
  String trangThai = canhBaoQuaTai
    ? (cheDoChangBat ? "QUA TAI — CUA BI CHAN" : "QUA TAI — CHE DO CHAN TAT")
    : (soNguoiHienTai == 0 ? "TRONG" : "HOAT DONG");
  String mauTT = canhBaoQuaTai ? "#e74c3c" : soNguoiHienTai == 0 ? "#7f8c8d" : "#27ae60";
  String cheDoBtn = cheDoChangBat
    ? "<a class='btn btn-on'  href='/togglechan'>Che do chan: BAT</a>"
    : "<a class='btn btn-off' href='/togglechan'>Che do chan: TAT</a>";
  String ghiDeBtn = (canhBaoQuaTai && cheDoChangBat)
    ? "<a class='btn btn-override' href='/ghide'>Ghi de — cho vao 1 luot</a>" : "";

  String html = R"rawhtml(
<!DOCTYPE html><html lang='vi'><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<meta http-equiv='refresh' content='2'>
<title>People Counter AIoT</title>
<style>
  *{margin:0;padding:0;box-sizing:border-box}
  body{font-family:'Segoe UI',sans-serif;background:#0f0f1a;color:#fff;min-height:100vh;padding:20px}
  .wrap{max-width:600px;margin:0 auto}
  h1{text-align:center;font-size:1.1rem;color:#aaa;letter-spacing:3px;text-transform:uppercase;margin-bottom:20px}
  .card{background:#1a1a2e;border-radius:16px;padding:22px;margin-bottom:14px;border:1px solid #2a2a4a}
  .big{font-size:5.5rem;font-weight:700;text-align:center;line-height:1}
  .lbl{text-align:center;color:#888;font-size:.8rem;letter-spacing:2px;margin-top:6px;text-transform:uppercase}
  .badge{display:inline-block;padding:5px 16px;border-radius:99px;font-size:.78rem;font-weight:600;letter-spacing:1px;margin-top:10px}
  .center{text-align:center}
  .bar-wrap{background:#0a0a15;border-radius:99px;height:10px;margin:14px 0 4px}
  .bar-fill{height:10px;border-radius:99px}
  .grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px}
  .stat{background:#12122a;border-radius:12px;padding:14px;text-align:center}
  .sn{font-size:1.9rem;font-weight:700}
  .sl{color:#666;font-size:.72rem;letter-spacing:1px;margin-top:3px;text-transform:uppercase}
  .cg{color:#27ae60}.cr{color:#e74c3c}.cb{color:#3498db}
  .btn{display:block;width:100%;padding:11px;border-radius:10px;text-decoration:none;
       font-size:.88rem;font-weight:600;text-align:center;border:1px solid #333;
       color:#ccc;background:#1a1a2e;margin-bottom:8px}
  .btn:hover{background:#2a2a4e}
  .btn-on{border-color:#27ae60;color:#27ae60;background:#0a1f0f}
  .btn-off{border-color:#e74c3c;color:#e74c3c;background:#1f0a0a}
  .btn-override{border-color:#f39c12;color:#f39c12;background:#1f180a}
  .row{display:flex;gap:8px;flex-wrap:wrap;margin-top:8px}
  .btn-sm{flex:1;padding:9px 6px;margin:0}
  .ai-box{background:#0a1520;border:1px solid #1f6feb;border-radius:10px;padding:14px;margin-top:10px;font-size:.82rem}
  .foot{text-align:center;color:#333;font-size:.72rem;margin-top:14px}
</style></head><body>
<div class='wrap'>
<h1>🛡️ People Counter AIoT</h1>
<div class='card'>
  <div class='big'>)rawhtml";

  html += String(soNguoiHienTai);
  html += R"rawhtml(</div>
  <div class='lbl'>nguoi dang trong phong</div>
  <div class='center' style='margin-top:10px'>
    <span class='badge' style='background:)rawhtml";
  html += mauTT + "22;color:" + mauTT + ";border:1px solid " + mauTT + "'>";
  html += trangThai + "</span></div>";
  html += "<div class='bar-wrap'><div class='bar-fill' style='width:" + String(phanTram);
  html += "%;background:" + mauBar + "'></div></div>";
  html += "<div style='display:flex;justify-content:space-between;font-size:.72rem;color:#444'>";
  html += "<span>0</span><span>Gioi han: " + String(GIOI_HAN_PHONG) + " nguoi</span></div>";
  html += "<div class='ai-box'>🤖 <b>AI Backend:</b> <a href='http://172.16.23.193:8000' style='color:#58a6ff'>Xem Dashboard AI</a> &nbsp;|&nbsp; ";
  html += "Signal: " + String(WiFi.RSSI()) + " dBm</div>";

  html += R"rawhtml(
</div>
<div class='card'>
  <div class='grid'>
    <div class='stat'><div class='sn cg'>)rawhtml";
  html += String(soNguoiVao);
  html += R"rawhtml(</div><div class='sl'>Tong vao</div></div>
    <div class='stat'><div class='sn cr'>)rawhtml";
  html += String(soNguoiRa);
  html += R"rawhtml(</div><div class='sl'>Tong ra</div></div>
    <div class='stat'><div class='sn cb'>)rawhtml";
  html += String(GIOI_HAN_PHONG);
  html += R"rawhtml(</div><div class='sl'>Gioi han</div></div>
  </div>
</div>
<div class='card'>)rawhtml";
  html += cheDoBtn;
  html += ghiDeBtn;
  html += R"rawhtml(
  <div class='row'>
    <a class='btn btn-sm' href='/reset'>Reset dem</a>
    <a class='btn btn-sm' href='/test'>Test buzzer</a>
    <a class='btn btn-sm' href='/setlimit?v=5'>Max 5</a>
    <a class='btn btn-sm' href='/setlimit?v=10'>Max 10</a>
    <a class='btn btn-sm' href='/setlimit?v=20'>Max 20</a>
  </div>
</div>
<div class='foot'>)rawhtml";
  html += "Uptime: " + String(uptime/3600) + "h " + String((uptime%3600)/60) + "m " + String(uptime%60) + "s";
  html += " | " + WiFi.localIP().toString();
  html += " | Auto refresh 2s</div></div></body></html>";
  return html;
}

// ============================================================
// WEB SERVER ROUTES
// ============================================================
void setupWebServer() {
  server.on("/", []() { server.send(200, "text/html", trangChu()); });

  server.on("/status", []() {
    int phanTram = GIOI_HAN_PHONG > 0
                   ? min(soNguoiHienTai * 100 / GIOI_HAN_PHONG, 100) : 0;
    String j = "{\"device_id\":\"" + String(DEVICE_ID) + "\""
             + ",\"zone_id\":\"" + String(ZONE_ID) + "\""
             + ",\"so_nguoi_hien_tai\":" + String(soNguoiHienTai)
             + ",\"so_nguoi_vao\":"  + String(soNguoiVao)
             + ",\"so_nguoi_ra\":"   + String(soNguoiRa)
             + ",\"gioi_han_phong\":" + String(GIOI_HAN_PHONG)
             + ",\"phan_tram_su_dung\":" + String(phanTram)
             + ",\"canh_bao_qua_tai\":" + String(canhBaoQuaTai ? "true":"false")
             + ",\"che_do_chan\":" + String(cheDoChangBat ? "true":"false")
             + ",\"network_signal\":" + String(WiFi.RSSI())
             + "}";
    server.send(200, "application/json", j);
  });

  server.on("/togglechan", []() {
    cheDoChangBat = !cheDoChangBat;
    if (!cheDoChangBat) servo.write(SERVO_DONG);
    guiTelemetry("toggle_chan");
    veOLED();
    server.sendHeader("Location", "/"); server.send(302, "text/plain", "");
  });

  server.on("/ghide", []() {
    if (canhBaoQuaTai && cheDoChangBat) {
      ghiDeQuaTai = true;
      moCua();
      guiTelemetry("ghi_de");
    }
    server.sendHeader("Location", "/"); server.send(302, "text/plain", "");
  });

  server.on("/reset", []() {
    soNguoiVao = soNguoiRa = soNguoiHienTai = 0;
    canhBaoQuaTai = false; ghiDeQuaTai = false;
    servo.write(SERVO_DONG);
    guiTelemetry("reset");
    veOLED();
    server.sendHeader("Location", "/"); server.send(302, "text/plain", "");
  });

  server.on("/test", []() {
    beepVao();
    server.sendHeader("Location", "/"); server.send(302, "text/plain", "");
  });

  server.on("/setlimit", []() {
    if (server.hasArg("v")) {
      GIOI_HAN_PHONG = server.arg("v").toInt();
      canhBaoQuaTai  = (soNguoiHienTai >= GIOI_HAN_PHONG);
      guiTelemetry("set_limit");
    }
    server.sendHeader("Location", "/"); server.send(302, "text/plain", "");
  });

  server.begin();
  Serial.println("[WEB] Server đang chạy!");
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  tgBatDau = millis();

  pinMode(PIN_IR,     INPUT);
  pinMode(PIN_LED,    OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_LED,    LOW);
  digitalWrite(PIN_BUZZER, LOW);

  servo.attach(PIN_SERVO);
  servo.write(SERVO_DONG);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
    Serial.println("[!] Không tìm thấy OLED!");
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setTextSize(1);
  oled.setCursor(5, 20); oled.print("PEOPLE COUNTER AIoT");
  oled.setCursor(20, 35); oled.print("Dang ket noi...");
  oled.display();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[WiFi] Đang kết nối");
  int thu = 0;
  while (WiFi.status() != WL_CONNECTED && thu < 40) {
    delay(500); Serial.print("."); thu++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] IP: " + WiFi.localIP().toString());
    oled.clearDisplay();
    oled.setCursor(0, 10); oled.print("WiFi: OK");
    oled.setCursor(0, 25); oled.print(WiFi.localIP().toString());
    oled.display(); delay(2000);
    setupWebServer();
    guiTelemetry("startup");
  } else {
    Serial.println("\n[WiFi] Offline mode!");
    oled.clearDisplay();
    oled.setCursor(10, 25); oled.print("OFFLINE MODE");
    oled.display(); delay(1500);
  }

  beepVao();
  veOLED();
  Serial.println("[OK] Sẵn sàng! r=reset | -=người ra | t=toggle chặn");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  server.handleClient();

  bool trangThaiIR = digitalRead(PIN_IR);

  // Phát hiện người vào (IR cắt chùm tia)
  if (trangThaiIR_truoc == HIGH && trangThaiIR == LOW) {
    delay(50);
    if (digitalRead(PIN_IR) == LOW) {
      if (canhBaoQuaTai && cheDoChangBat && !ghiDeQuaTai) {
        Serial.println("[!] Từ chối — phòng quá tải, cửa bị chặn!");
        servo.write(SERVO_CHAN);
        beepCanhBao();
        guiTelemetry("tu_choi_qua_tai");
      } else {
        soNguoiVao++;
        soNguoiHienTai++;
        Serial.println(">>> VÀO! Hiện tại: " + String(soNguoiHienTai));
        canhBaoQuaTai = (soNguoiHienTai >= GIOI_HAN_PHONG);
        canhBaoQuaTai ? beepCanhBao() : beepVao();
        moCua();
        guiTelemetry("nguoi_vao");
      }
      veOLED();
    }
  }
  trangThaiIR_truoc = trangThaiIR;

  // Serial commands
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'r' || c == 'R') {
      soNguoiVao = soNguoiRa = soNguoiHienTai = 0;
      canhBaoQuaTai = false; ghiDeQuaTai = false;
      servo.write(SERVO_DONG);
      guiTelemetry("reset");
      Serial.println("[RESET] Done!");
      veOLED();
    }
    if (c == '-') {
      if (soNguoiHienTai > 0) {
        soNguoiRa++; soNguoiHienTai--;
        canhBaoQuaTai = (soNguoiHienTai >= GIOI_HAN_PHONG);
        beepRa();
        Serial.println("<<< RA! Hiện tại: " + String(soNguoiHienTai));
        guiTelemetry("nguoi_ra");
        veOLED();
      }
    }
    if (c == 't' || c == 'T') {
      cheDoChangBat = !cheDoChangBat;
      if (!cheDoChangBat) servo.write(SERVO_DONG);
      guiTelemetry("toggle_chan");
      veOLED();
    }
  }

  // FIX: Tự động reset cảnh báo quá tải khi phòng thực sự trống
  if (soNguoiHienTai == 0 && canhBaoQuaTai) {
    canhBaoQuaTai = false;
    ghiDeQuaTai   = false;
    servo.write(SERVO_DONG);
    Serial.println("[AUTO] Phòng trống → reset cảnh báo quá tải");
    veOLED();
  }

  // Gửi định kỳ mỗi 10 giây
  if (millis() - tgGuiBackend >= tgGuiInterval) {
    tgGuiBackend = millis();
    guiTelemetry("periodic");
  }

  // Nhận lệnh từ Dashboard mỗi 3 giây
  if (millis() - tgNhanLenh >= 3000) {
    tgNhanLenh = millis();
    nhanLenh();
  }

  capNhatServo();
  capNhatLED();

  if (millis() - tgCapNhatOLED > 5000) {
    tgCapNhatOLED = millis();
    veOLED();
  }

  delay(20);
}
