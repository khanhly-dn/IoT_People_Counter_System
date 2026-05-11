USE master;
GO

IF NOT EXISTS (SELECT name FROM sys.databases WHERE name = 'PeopleCounter')
    CREATE DATABASE PeopleCounter;
GO

USE PeopleCounter;
GO

-- BẢNG 1: THIẾT BỊ
IF OBJECT_ID('dbo.devices','U') IS NOT NULL DROP TABLE dbo.devices;
CREATE TABLE dbo.devices (
    device_id       NVARCHAR(50)  PRIMARY KEY,
    name            NVARCHAR(100) NOT NULL,
    zone_id         NVARCHAR(50)  NOT NULL,
    location_desc   NVARCHAR(200),
    gioi_han_phong  INT           DEFAULT 10,
    status          NVARCHAR(20)  DEFAULT 'active',
    created_at      DATETIME      DEFAULT GETDATE()
);

-- BẢNG 2: TELEMETRY TỪ ESP32
IF OBJECT_ID('dbo.telemetry','U') IS NOT NULL DROP TABLE dbo.telemetry;
CREATE TABLE dbo.telemetry (
    id                  INT           IDENTITY(1,1) PRIMARY KEY,
    device_id           NVARCHAR(50)  NOT NULL,
    zone_id             NVARCHAR(50),
    timestamp           DATETIME      DEFAULT GETDATE(),
    so_nguoi_hien_tai   INT           DEFAULT 0,
    so_nguoi_vao        INT           DEFAULT 0,
    so_nguoi_ra         INT           DEFAULT 0,
    gioi_han_phong      INT           DEFAULT 10,
    phan_tram_su_dung   INT           DEFAULT 0,  
    canh_bao_qua_tai    BIT           DEFAULT 0,
    che_do_chan         BIT           DEFAULT 1,
    cua_dang_mo         BIT           DEFAULT 0,
    battery_level       FLOAT         DEFAULT 100.0,
    network_signal      INT           DEFAULT -50,
    ly_do_gui           NVARCHAR(50),  
    FOREIGN KEY (device_id) REFERENCES dbo.devices(device_id)
);

-- BẢNG 3: KẾT QUẢ AI
IF OBJECT_ID('dbo.ai_results','U') IS NOT NULL DROP TABLE dbo.ai_results;
CREATE TABLE dbo.ai_results (
    id                  INT           IDENTITY(1,1) PRIMARY KEY,
    telemetry_id        INT           NOT NULL,
    device_id           NVARCHAR(50)  NOT NULL,
    timestamp           DATETIME      DEFAULT GETDATE(),
    model_name          NVARCHAR(100),
    -- Module 1: Anomaly Detection
    trang_thai          NVARCHAR(20),  
    -- Module 2: Occupancy Prediction
    du_bao_30p          INT,           
    xu_huong            NVARCHAR(20), 
    -- Kết quả chung
    risk_level          NVARCHAR(20),  
    risk_score          FLOAT          DEFAULT 0.0,
    reason              NVARCHAR(500),
    recommended_action  NVARCHAR(200),
    FOREIGN KEY (telemetry_id) REFERENCES dbo.telemetry(id),
    FOREIGN KEY (device_id)   REFERENCES dbo.devices(device_id)
);

-- BẢNG 4: SỰ KIỆN CẢNH BÁO
IF OBJECT_ID('dbo.events','U') IS NOT NULL DROP TABLE dbo.events;
CREATE TABLE dbo.events (
    id          INT           IDENTITY(1,1) PRIMARY KEY,
    device_id   NVARCHAR(50)  NOT NULL,
    zone_id     NVARCHAR(50),
    event_type  NVARCHAR(100), -- QUA_TAI / DU_BAO_QUA_TAI / TRONG_LAU / TU_CHOI
    risk_level  NVARCHAR(20),
    description NVARCHAR(500),
    resolved    BIT           DEFAULT 0,
    created_at  DATETIME      DEFAULT GETDATE(),
    FOREIGN KEY (device_id) REFERENCES dbo.devices(device_id)
);

-- BẢNG 5: LỆNH ĐIỀU KHIỂN
IF OBJECT_ID('dbo.commands','U') IS NOT NULL DROP TABLE dbo.commands;
CREATE TABLE dbo.commands (
    id           INT           IDENTITY(1,1) PRIMARY KEY,
    device_id    NVARCHAR(50)  NOT NULL,
    command      NVARCHAR(100) NOT NULL,  -- BLOCK_ENTRY / ALLOW_ENTRY / SET_LIMIT
    payload      NVARCHAR(200),
    status       NVARCHAR(20)  DEFAULT 'pending',
    triggered_by NVARCHAR(50)  DEFAULT 'AI',
    created_at   DATETIME      DEFAULT GETDATE(),
    executed_at  DATETIME,
    FOREIGN KEY (device_id) REFERENCES dbo.devices(device_id)
);

-- DỮ LIỆU MẪU
INSERT INTO dbo.devices VALUES
('people_counter_zone_A', N'Bộ đếm người Cửa A', 'zone_A_entrance',
 N'Cửa vào chính tầng 1', 10, 'active', GETDATE()),
('people_counter_zone_B', N'Bộ đếm người Cửa B', 'zone_B_exit',
 N'Cửa thoát hiểm tầng 1', 10, 'active', GETDATE());

-- INDEX
CREATE INDEX IX_telemetry_device_time ON dbo.telemetry(device_id, timestamp DESC);
CREATE INDEX IX_ai_results_device     ON dbo.ai_results(device_id, timestamp DESC);
CREATE INDEX IX_events_unresolved     ON dbo.events(resolved, created_at DESC);

PRINT 'PeopleCounter database tao thanh cong!';
GO

USE PeopleCounter;
SELECT * FROM telemetry ORDER BY timestamp DESC;
