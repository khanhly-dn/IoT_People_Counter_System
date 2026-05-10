# 🚪 IoT People Counter System

<p align="center">
  <img src="https://img.shields.io/badge/Platform-ESP32-blue?style=for-the-badge&logo=espressif" />
  <img src="https://img.shields.io/badge/Sensor-IR%20FC--51-orange?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Protocol-WiFi%20%7C%20HTTP-green?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Actuator-Servo%20Motor-purple?style=for-the-badge" />
  <img src="https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge" />
</p>

<p align="center">
  Hệ thống <strong>đếm và kiểm soát người ra vào phòng</strong> thông minh sử dụng <strong>ESP32 + IR FC-51</strong>,  
  tích hợp <strong>Web Dashboard</strong>, <strong>OLED</strong>, <strong>Buzzer</strong>, <strong>LED</strong> và <strong>Servo Motor</strong>.
</p>

---

## 📌 Giới thiệu

Dự án **IoT People Counter System** được phát triển nhằm xây dựng hệ thống **đếm số người và kiểm soát ra vào phòng** theo thời gian thực.  
Hệ thống liên tục đọc tín hiệu từ cảm biến **IR FC-51**, phân tích và phản hồi ngay lập tức thông qua:

- 🌐 **Web Dashboard** hiện đại, tự cập nhật mỗi 2 giây, truy cập từ bất kỳ thiết bị nào trong cùng mạng WiFi
- 📟 **Màn hình OLED** hiển thị số người, tổng vào/ra và trạng thái chế độ chặn trực tiếp
- 🔔 **Buzzer** cảnh báo âm thanh theo từng tình huống (vào, ra, quá tải)
- 💡 **LED trạng thái** báo hiệu trực quan: sáng khi có người, nhấp nháy khi quá tải
- 🔒 **Servo Motor** tự động mở/đóng cửa và chặn cửa khi phòng đạt giới hạn

---

## ⚙️ Chức năng chính

- **Đếm người ra vào** liên tục qua cảm biến IR FC-51 (GPIO34)
- **Chế độ chặn thông minh** – Servo tự động chặn cửa 45° khi phòng quá tải
- **Ghi đè linh hoạt** – Cho phép vào 1 lượt dù đang quá tải qua nút trên Web
- **Buzzer thông minh** – 2 bíp khi vào, 1 bíp dài khi ra, 4 bíp nhanh khi quá tải
- **OLED hiển thị** – Số người hiện tại (cỡ chữ lớn), tổng vào/ra, trạng thái chặn
- **Web Dashboard** – Giám sát realtime, bật/tắt chế độ chặn, ghi đè, reset, đặt giới hạn
- **Servo Motor** – Mở 90° khi có người, giữ 3 giây rồi đóng về 0°; chặn 45° khi quá tải
- **Giới hạn phòng tùy chỉnh** – Đặt được Max 5 / 10 / 20 người trực tiếp trên Web

---

## 🧩 Sơ đồ hoạt động

<p align="center">
  <img width="500" alt="Sơ đồ hoạt động" src="https://github.com/khanhly-dn/IoT_People_Counter_System/blob/main/SDHD.png?raw=true" />
</p>

```
Cảm biến IR FC-51 → ESP32 phát hiện vật cản HIGH → LOW
        ↓
  Quá tải + chặn BẬT  →  Servo 45° chặn cửa · Buzzer cảnh báo
  Bình thường          →  Đếm +1 · Servo mở 90° (3s) · Buzzer 2 bíp
        ↓
  Chạm giới hạn  →  LED nhấp nháy · OLED cảnh báo · chặn lượt tiếp theo
  Chưa chạm      →  LED sáng · OLED cập nhật số người
        ↓
  Web Dashboard tự refresh mỗi 2 giây
```

---

## 🛠️ Phần cứng sử dụng

| Linh kiện | Chân kết nối | Mô tả |
|---|---|---|
| **ESP32** | – | Vi điều khiển chính, WiFi tích hợp |
| **Cảm biến IR FC-51** | DO → GPIO34 | Phát hiện người đi qua cửa |
| **Servo Motor** | Signal → GPIO18 | Mở/đóng/chặn cửa tự động |
| **LED trạng thái** | GPIO2 | Báo hiệu có người / quá tải |
| **Buzzer** | GPIO5 | Cảnh báo âm thanh |
| **OLED SSD1306** | SDA → GPIO21, SCL → GPIO22 (I²C, 0x3C) | Hiển thị trạng thái 128×64 |
| **Nguồn** | 5V USB | Cấp điện toàn bộ hệ thống |

<p align="center">
  <img width="600" alt="Sơ đồ linh kiện" src="https://github.com/khanhly-dn/IoT_People_Counter_System/blob/main/TB.png?raw=true" />
</p>

---

## 🔌 Sơ đồ nối dây

| Linh kiện | Chân linh kiện | Chân ESP32 |
|---|---|---|
| **IR FC-51** | DO | GPIO34 |
| **IR FC-51** | VCC | 3.3V |
| **IR FC-51** | GND | GND |
| **Servo Motor** | Signal (cam) | GPIO18 |
| **Servo Motor** | VCC (đỏ) | 5V |
| **Servo Motor** | GND (nâu) | GND |
| **LED** | Anode (+) | GPIO2 (qua điện trở 220Ω) |
| **LED** | Cathode (−) | GND |
| **Buzzer** | (+) | GPIO5 |
| **Buzzer** | (−) | GND |
| **OLED SSD1306** | SDA | GPIO21 |
| **OLED SSD1306** | SCL | GPIO22 |
| **OLED SSD1306** | VCC | 3.3V |
| **OLED SSD1306** | GND | GND |

> ⚠️ LED cần nối tiếp điện trở **220Ω**. Buzzer dùng loại **Active** (tự dao động). Tất cả GND nối chung một điểm.

---

## 💻 Phần mềm & Công nghệ

- **Ngôn ngữ:** Arduino C++, HTML/CSS/JavaScript
- **Framework:** Arduino ESP32 Core
- **Thư viện:**
  - `WiFi.h` – kết nối mạng WiFi
  - `WebServer.h` – HTTP server nội bộ nhúng trong ESP32
  - `ESP32Servo` – điều khiển Servo Motor
  - `Adafruit_SSD1306` + `Adafruit_GFX` – điều khiển màn hình OLED
- **Giao tiếp:** HTTP REST API qua WiFi nội bộ
- **Giao diện:** Web App nhúng trực tiếp trong ESP32 (không cần server ngoài)

---

## 🌐 API Endpoints

| Endpoint | Mô tả |
|---|---|
| `GET /` | Giao diện Web Dashboard chính |
| `GET /status` | Trả về JSON: số người, tổng vào/ra, giới hạn, trạng thái chặn |
| `GET /togglechan` | Bật/tắt chế độ chặn cửa khi quá tải |
| `GET /ghide` | Ghi đè – cho vào 1 lượt dù đang quá tải |
| `GET /reset` | Reset toàn bộ bộ đếm về 0 |
| `GET /test` | Test buzzer |
| `GET /setlimit?v=N` | Đặt giới hạn số người tối đa |

---

## 📊 Thông số kỹ thuật

| Thông số | Giá trị |
|---|---|
| Góc Servo mở cửa | 90° |
| Góc Servo đóng cửa | 0° |
| Góc Servo chặn cửa (quá tải) | 45° |
| Thời gian giữ mở cửa | 3 giây |
| Giới hạn người mặc định | 10 người |
| Debounce cảm biến IR | 50ms |
| Tần suất refresh Web | 2 giây / lần |
| Cổng WebServer | 80 (HTTP) |
| Màn hình OLED | 128×64px, I²C 0x3C |
| Tốc độ Serial Monitor | 115200 baud |

---

## 🚀 Hướng dẫn cài đặt

**1. Cài đặt môi trường**
```bash
# Cài Arduino IDE >= 2.0
# Thêm ESP32 board vào Board Manager:
# https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

**2. Cài thư viện** (Arduino Library Manager)
```
Adafruit SSD1306
Adafruit GFX Library
ESP32Servo
```

**3. Cấu hình trong code**
```cpp
#define WIFI_SSID   "Tên_WiFi_của_bạn"
#define WIFI_PASS   "Mật_khẩu_WiFi"
#define GIOI_HAN_PHONG  10   // Số người tối đa
```

**4. Nạp code & chạy**
```
1. Mở file people_counter.ino trong Arduino IDE
2. Chọn đúng board: ESP32 Dev Module
3. Nạp code qua cáp USB
4. Mở Serial Monitor (115200 baud) để xem địa chỉ IP
5. Truy cập địa chỉ IP trên trình duyệt → Web Dashboard xuất hiện
```

**5. Phím tắt trong Serial Monitor**
```
r  →  Reset bộ đếm về 0
-  →  Giả lập 1 người ra (dùng khi test)
t  →  Bật/tắt chế độ chặn
```

---

## 📷 Demo sản phẩm

<p align="center">
  <img width="700" alt="Demo Web Dashboard" src="https://github.com/khanhly-dn/IoT_People_Counter_System/blob/main/DEMO.jpg?raw=true" />
</p>

---

## 🚀 Hướng phát triển

- [ ] Thêm **sensor IR thứ hai** để tự động phân biệt người vào / ra
- [ ] Tích hợp **MQTT** để kết nối Home Assistant / Node-RED
- [ ] Gửi thông báo **Telegram** khi phòng quá tải
- [ ] **OTA Update** – cập nhật firmware qua WiFi
- [ ] Thêm **lịch sử biểu đồ** số người theo thời gian thực
- [ ] Hỗ trợ **nhiều cửa / nhiều phòng** trên một dashboard

---

## 👤 Thực hiện

**Lý Gia Khánh**  
Khoa Công nghệ Thông tin – Trường Đại học Đại Nam

---

<p align="center">
  Using ESP32 · IR FC-51 · Servo Motor · Arduino
</p>
