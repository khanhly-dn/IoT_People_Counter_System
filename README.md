# 🚪 IoT People Counter — AIoT Edition

<p align="center">
  <img src="https://img.shields.io/badge/Platform-ESP32-blue?style=for-the-badge&logo=espressif" />
  <img src="https://img.shields.io/badge/Sensor-IR%20FC--51-orange?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Backend-FastAPI-009688?style=for-the-badge&logo=fastapi" />
  <img src="https://img.shields.io/badge/AI-Anomaly%20%7C%20Prediction-blueviolet?style=for-the-badge&logo=python" />
  <img src="https://img.shields.io/badge/Database-SQL%20Server-CC2927?style=for-the-badge&logo=microsoftsqlserver" />
  <img src="https://img.shields.io/badge/Protocol-WiFi%20%7C%20HTTP%20REST-green?style=for-the-badge" />
  <img src="https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge" />
</p>

<p align="center">
  Hệ thống <strong>đếm và kiểm soát người ra vào phòng thông minh</strong> thế hệ mới —<br/>
  Kết hợp <strong>ESP32 + IR FC-51</strong> ở tầng thiết bị với <strong>FastAPI + AI Engine + SQL Server</strong> ở tầng cloud/server,<br/>
  tạo thành một giải pháp <strong>AIoT hoàn chỉnh</strong>: thu thập dữ liệu → phân tích AI → điều khiển tự động → dashboard realtime.
</p>

---

## 📌 Giới thiệu

**IoT People Counter — AIoT Edition** là bản nâng cấp toàn diện từ hệ thống đếm người nhúng ESP32 thuần túy lên kiến trúc **AIoT phân tầng** gồm:

- **Tầng thiết bị (Edge):** ESP32 + cảm biến IR FC-51 đọc tín hiệu, đếm người, điều khiển servo/buzzer/OLED ngay tại cửa
- **Tầng server:** FastAPI (Python) nhận telemetry, chạy AI pipeline, lưu trữ SQL Server, phát lệnh điều khiển ngược về ESP32
- **Tầng AI:** Hai module độc lập — Anomaly Detection (phát hiện bất thường) và Occupancy Prediction (dự báo xu hướng) — kết hợp thành Risk Classifier đưa ra quyết định tự động
- **Tầng giám sát:** Web Dashboard tự cập nhật realtime, hiển thị số người, mức rủi ro, lịch sử, sự kiện cảnh báo

---

## 🏗️ Kiến trúc hệ thống

```
┌─────────────────────────────────┐
│        TẦNG THIẾT BỊ (Edge)     │
│  IR FC-51 → ESP32               │
│  ├─ OLED SSD1306 (128×64)       │
│  ├─ Servo Motor (GPIO18)        │
│  ├─ Buzzer (GPIO5)              │
│  └─ LED trạng thái (GPIO2)      │
└────────────┬────────────────────┘
             │ HTTP REST (WiFi)
             ▼
┌─────────────────────────────────┐
│        TẦNG SERVER              │
│  FastAPI (app.py)               │
│  ├─ POST /api/telemetry         │
│  ├─ GET  /api/commands/pending  │
│  └─ POST /api/commands/{id}/done│
└────────────┬────────────────────┘
             │
     ┌───────┴────────┐
     ▼                ▼
┌──────────┐   ┌──────────────────┐
│ AI Engine│   │  SQL Server DB   │
│          │   │  ├─ devices      │
│ Module 1 │   │  ├─ telemetry    │
│ Anomaly  │   │  ├─ ai_results   │
│ Detection│   │  ├─ events       │
│          │   │  └─ commands     │
│ Module 2 │   └──────────────────┘
│ Occupancy│
│ Predict  │
│          │
│ Risk     │
│ Classifier│
└──────────┘
             │
             ▼
┌─────────────────────────────────┐
│       WEB DASHBOARD             │
│  Giám sát realtime · Điều khiển │
│  Biểu đồ lịch sử · Cảnh báo    │
└─────────────────────────────────┘
```

---

## ✨ Tính năng nổi bật

### 🔩 Phần cứng (ESP32)
- **Đếm người ra vào** liên tục qua cảm biến IR FC-51 (GPIO34)
- **Servo Motor** tự động mở 90° khi có người, giữ 3 giây rồi đóng về 0°; chặn 45° khi phòng quá tải
- **Buzzer thông minh** — 2 bíp khi vào, 1 bíp dài khi ra, 4 bíp nhanh khi quá tải
- **OLED SSD1306** hiển thị số người hiện tại (chữ lớn), tổng vào/ra, trạng thái chặn
- **LED trạng thái** — sáng khi có người, nhấp nháy nhanh khi quá tải
- **Nhận lệnh từ server** — ESP32 polling lệnh pending mỗi 5 giây, tự thực thi và báo lại

### 🤖 AI Engine (Python)
- **Module 1 — Anomaly Detection:** Phát hiện quá tải phòng, pin yếu, tín hiệu WiFi kém, sự kiện từ chối người vào
- **Module 2 — Occupancy Prediction:** Dự báo số người trong 30 phút tới dựa trên giờ cao điểm và xu hướng hiện tại
- **Risk Classifier:** Kết hợp cả hai module, tính risk_score 0–100, phân loại NONE / LOW / MEDIUM / HIGH, đề xuất hành động tự động
- **Chống vòng lặp:** Tính lại `phan_tram` từ số người thực tế thay vì dùng cờ `canh_bao_qua_tai` từ ESP32

### 🌐 Backend FastAPI
- **9 API endpoints** đầy đủ: nhận telemetry, lấy dữ liệu mới nhất, lịch sử, sự kiện, điều khiển thủ công, quản lý lệnh
- **Tự động tạo lệnh BLOCK_ENTRY** khi AI đánh giá rủi ro HIGH, tự huỷ khi phòng trống
- **Lưu toàn bộ pipeline AI** vào DB: anomaly, prediction, risk level, reason, recommended_action
- **Chống duplicate lệnh** — kiểm tra lệnh pending trước khi thêm mới

### 🗄️ SQL Server Database
- **5 bảng liên kết:** `devices`, `telemetry`, `ai_results`, `events`, `commands`
- Index tối ưu cho truy vấn realtime theo device + timestamp
- Lưu đầy đủ lịch sử mọi lần đếm và quyết định AI

---

## 🧩 Sơ đồ hoạt động

<p align="center">
  <img width="500" alt="Sơ đồ hoạt động" src="https://github.com/khanhly-dn/IoT_People_Counter_System/blob/main/SDHD.png?raw=true" />
</p>

```
Cảm biến IR FC-51 → ESP32 phát hiện vật cản HIGH → LOW
        │
        ├─ POST /api/telemetry → FastAPI nhận
        │         │
        │         ├─ AI Pipeline chạy tức thì
        │         │   ├─ Anomaly Detection → NORMAL/WARNING/CRITICAL
        │         │   ├─ Occupancy Prediction → dự báo 30 phút
        │         │   └─ Risk Classifier → NONE/LOW/MEDIUM/HIGH + action
        │         │
        │         ├─ Lưu telemetry + ai_results + events vào SQL Server
        │         └─ Tạo lệnh BLOCK_ENTRY nếu risk = HIGH
        │
        └─ ESP32 polling GET /api/commands/pending mỗi 5s
                  │
                  ├─ Nhận BLOCK_ENTRY → Servo 45° + Buzzer cảnh báo
                  └─ POST /api/commands/{id}/done → đánh dấu đã thực thi

Web Dashboard ← GET /api/devices/{id}/latest (realtime)
             ← GET /api/events (cảnh báo)
             ← GET /api/devices/{id}/history (biểu đồ)
```

---

## 🛠️ Phần cứng

| Linh kiện | Chân kết nối | Mô tả |
|---|---|---|
| **ESP32** | — | Vi điều khiển chính, WiFi tích hợp |
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

| Tầng | Công nghệ | Phiên bản |
|---|---|---|
| **Firmware** | Arduino C++ / ESP32 Core | >= 2.0 |
| **Backend** | Python + FastAPI | FastAPI 0.111.0 |
| **ASGI Server** | Uvicorn | 0.29.0 |
| **Database** | Microsoft SQL Server | >= 2019 |
| **DB Driver** | pyodbc | 5.1.0 |
| **AI Engine** | Python thuần (không cần GPU) | — |
| **Data Validation** | Pydantic | 2.7.1 |
| **Config** | python-dotenv | 1.0.1 |

**Thư viện Arduino:**
- `WiFi.h` — kết nối mạng WiFi
- `WebServer.h` — HTTP REST polling
- `ESP32Servo` — điều khiển Servo Motor
- `Adafruit_SSD1306` + `Adafruit_GFX` — màn hình OLED

---

## 🌐 API Endpoints (FastAPI Backend)

| Method | Endpoint | Mô tả |
|---|---|---|
| `GET` | `/` | Web Dashboard chính |
| `POST` | `/api/telemetry` | **[ESP32]** Gửi dữ liệu đếm → chạy AI pipeline |
| `GET` | `/api/devices/{id}/latest` | Lấy trạng thái + AI mới nhất theo thiết bị |
| `GET` | `/api/devices/{id}/history` | Lịch sử đếm người (vẽ biểu đồ) |
| `GET` | `/api/events` | Danh sách sự kiện cảnh báo chưa xử lý |
| `GET` | `/api/status` | Trạng thái tổng quan hệ thống |
| `POST` | `/api/control` | **[Dashboard]** Gửi lệnh thủ công |
| `GET` | `/api/commands/{id}/pending` | **[ESP32]** Lấy lệnh đang chờ thực thi |
| `POST` | `/api/commands/{id}/done` | **[ESP32]** Báo đã thực thi lệnh xong |
| `POST` | `/api/commands/clear` | Hủy toàn bộ lệnh pending (reset khi bị loop) |

---

## 🤖 AI Engine — Chi tiết

### Module 1: Anomaly Detection
Phân tích dữ liệu telemetry từng bản ghi, trả về `trang_thai`:

| Điều kiện | Trạng thái |
|---|---|
| `so_nguoi_hien_tai >= gioi_han_phong` | `CRITICAL` |
| `phan_tram_su_dung >= 80%` | `WARNING` |
| `ly_do_gui == "tu_choi_qua_tai"` | `CRITICAL` |
| `battery_level < 20%` | `WARNING` (nếu chưa có lỗi nghiêm trọng) |
| Bình thường | `NORMAL` |

### Module 2: Occupancy Prediction
Dự báo số người và xu hướng trong 30 phút tới dựa trên giờ trong ngày:

| Khung giờ | Xu hướng | Delta |
|---|---|---|
| 7h – 9h (giờ vào) | `TANG` | +20% giới hạn phòng |
| 11h – 13h (nghỉ trưa) | `GIAM` | −30% số hiện tại |
| 17h – 19h (tan làm) | `GIAM` | −40% số hiện tại |
| 20h – 6h (ngoài giờ) | `ON_DINH` | 0 |
| Các giờ còn lại | `ON_DINH` | +5% giới hạn |

### Risk Classifier

| Risk Score | Risk Level | Hành động tự động |
|---|---|---|
| ≥ 60 | `HIGH` | `BLOCK_ENTRY` + `SEND_ALERT` + `UPDATE_DASHBOARD` |
| 30 – 59 | `MEDIUM` | `WARN_USER` + `SEND_NOTIFICATION` |
| 10 – 29 | `LOW` | `LOG_EVENT` + `UPDATE_DASHBOARD` |
| < 10 | `NONE` | `LOG_ONLY` |

---

## 🗄️ Cơ sở dữ liệu (SQL Server)

### Sơ đồ quan hệ

```
devices (device_id PK)
    ↑
    ├── telemetry (id PK, device_id FK)
    │       ↑
    │       └── ai_results (id PK, telemetry_id FK, device_id FK)
    │
    ├── events (id PK, device_id FK)
    └── commands (id PK, device_id FK)
```

### Mô tả bảng

| Bảng | Mô tả | Cột quan trọng |
|---|---|---|
| `devices` | Thông tin thiết bị ESP32 | `device_id`, `zone_id`, `gioi_han_phong` |
| `telemetry` | Dữ liệu thô từ ESP32 | `so_nguoi_hien_tai`, `phan_tram_su_dung`, `ly_do_gui` |
| `ai_results` | Kết quả AI pipeline | `trang_thai`, `du_bao_30p`, `xu_huong`, `risk_level`, `risk_score` |
| `events` | Sự kiện cảnh báo | `event_type`, `risk_level`, `resolved` |
| `commands` | Lệnh điều khiển | `command`, `status`, `triggered_by`, `executed_at` |

---

## 📷 Demo sản phẩm

<p align="center">
  <img width="700" alt="Demo Web Dashboard" src="https://github.com/khanhly-dn/IoT_People_Counter_System/blob/main/DEMO.jpg?raw=true" />
</p>

### 🖥️ Giao diện Web Dashboard

<p align="center">
  <img width="700" alt="Giao diện Web Dashboard" src="https://github.com/khanhly-dn/AIoT_People_Counter_System-DEMO/blob/main/GD.png?raw=true" />
</p>

### 📊 Dữ liệu & Biểu đồ thống kê

<p align="center">
  <img width="700" alt="Dữ liệu và biểu đồ thống kê" src="https://github.com/khanhly-dn/AIoT_People_Counter_System-DEMO/blob/main/DL.png?raw=true" />
</p>

🎬 **Video hoạt động:** *(đang cập nhật)*

---

## 🚀 Hướng dẫn cài đặt

### Yêu cầu hệ thống
- Windows với SQL Server 2019+ đã cài và chạy
- Python 3.11+
- Arduino IDE >= 2.0

### 1. Cài đặt Database

```sql
-- Chạy file schema.sql trong SQL Server Management Studio (SSMS)
-- File tự động tạo database PeopleCounter và toàn bộ bảng
```

### 2. Cài đặt Backend

```bash
# Clone repo
git clone https://github.com/khanhly-dn/AIoT_People_Counter_System-DEMO.git
cd AIoT_People_Counter_System-DEMO

# Cài dependencies
pip install -r requirements.txt

# Tạo file .env (tuỳ chỉnh nếu SQL Server không phải localhost)
echo "SQL_SERVER=localhost" > .env
echo "SQL_DATABASE=PeopleCounter" >> .env
echo "SQL_DRIVER=ODBC Driver 17 for SQL Server" >> .env

# Kiểm tra kết nối database
python connect_database.py

# Chạy server
uvicorn app:app --host 0.0.0.0 --port 8000 --reload
```

### 3. Cài đặt Firmware ESP32

```bash
# Cài Arduino IDE >= 2.0
# Thêm ESP32 board vào Board Manager:
# https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

**Cài thư viện** (Arduino Library Manager):
```
Adafruit SSD1306
Adafruit GFX Library
ESP32Servo
```

**Cấu hình trong `IoT_People_Counter_AIoT.ino`:**
```cpp
#define WIFI_SSID       "Tên_WiFi_của_bạn"
#define WIFI_PASS       "Mật_khẩu_WiFi"
#define SERVER_URL      "http://192.168.x.x:8000"   // IP máy chạy FastAPI
#define DEVICE_ID       "people_counter_zone_A"
#define GIOI_HAN_PHONG  10
```

**Nạp và chạy:**
```
1. Mở IoT_People_Counter_AIoT.ino trong Arduino IDE
2. Chọn board: ESP32 Dev Module
3. Nạp code qua cáp USB
4. Mở Serial Monitor (115200 baud) để xem log kết nối
5. Truy cập http://localhost:8000 để xem Dashboard
```

### 4. Kiểm tra hoạt động

```bash
# Gửi telemetry giả lập để test (PowerShell)
Invoke-RestMethod -Uri "http://localhost:8000/api/telemetry" `
  -Method POST -ContentType "application/json" `
  -Body '{"device_id":"people_counter_zone_A","so_nguoi_hien_tai":8,"gioi_han_phong":10}'

# Xem kết quả AI
Invoke-RestMethod "http://localhost:8000/api/devices/people_counter_zone_A/latest"
```

---

## ⚙️ Cấu hình & Thông số kỹ thuật

| Thông số | Giá trị |
|---|---|
| Góc Servo mở cửa | 90° |
| Góc Servo đóng cửa | 0° |
| Góc Servo chặn cửa (quá tải) | 45° |
| Thời gian giữ mở cửa | 3 giây |
| Giới hạn người mặc định | 10 người |
| Debounce cảm biến IR | 50ms |
| Tần suất ESP32 gửi telemetry | Mỗi sự kiện + 30s periodic |
| Tần suất ESP32 poll lệnh | 5 giây / lần |
| Tần suất refresh Dashboard | 2 giây / lần |
| Màn hình OLED | 128×64px, I²C 0x3C |
| Tốc độ Serial Monitor | 115200 baud |
| Backend port | 8000 |

---

## 📁 Cấu trúc thư mục

```
AIoT_People_Counter_System-DEMO/
│
├── IoT_People_Counter_AIoT.ino   # Firmware ESP32 (Arduino C++)
│
├── app.py                         # FastAPI server — 9 endpoints
├── connect_database.py            # Kết nối SQL Server (Windows Auth)
├── schema.sql                     # Tạo database + bảng + sample data
├── requirements.txt               # Python dependencies
│
├── ai/
│   └── ai_engine.py               # AI Pipeline (Anomaly + Prediction + Risk)
│
├── templates/
│   └── dashboard.html             # Web Dashboard (Jinja2)
│
├── static/                        # CSS, JS cho Dashboard
│
├── SDHD.png                       # Sơ đồ hoạt động
├── TB.png                         # Sơ đồ linh kiện
├── GD.png                         # Giao diện Web Dashboard
├── DL.png                         # Dữ liệu & biểu đồ thống kê
└── DEMO.jpg                       # Ảnh demo sản phẩm
```

---

## 🚧 Hướng phát triển

- [ ] Thêm **sensor IR thứ hai** để tự động phân biệt chiều người vào / ra chính xác hơn
- [ ] Tích hợp **MQTT** để giảm độ trễ, thay thế HTTP polling
- [ ] Gửi thông báo **Telegram Bot** khi phòng quá tải hoặc risk = HIGH
- [ ] **OTA Firmware Update** — cập nhật ESP32 qua WiFi không cần cáp USB
- [ ] **Machine Learning thực sự** — thay thế rule-based prediction bằng mô hình time-series (LSTM / Prophet)
- [ ] Hỗ trợ **nhiều phòng / nhiều thiết bị** trên một dashboard với bản đồ tòa nhà
- [ ] **Docker Compose** — đóng gói toàn bộ backend + DB để deploy dễ dàng

---

## 👤 Thực hiện

**Lý Gia Khánh**  
Khoa Công nghệ Thông tin — Trường Đại học Đại Nam

---

<p align="center">
  Built with ESP32 · IR FC-51 · FastAPI · SQL Server · Python AI · Arduino
</p>
