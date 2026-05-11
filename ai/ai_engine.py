# ============================================================
# ai_engine.py — AI cho People Counter
# Module 1: Anomaly Detection (phát hiện bất thường)
# Module 2: Occupancy Prediction (dự báo xu hướng)
# ============================================================
from datetime import datetime
from typing import Optional

# ============================================================
# MODULE 1: ANOMALY DETECTION
# Phát hiện các tình huống bất thường trong dữ liệu đếm người
# ============================================================
def run_anomaly_detection(telemetry: dict) -> dict:
    """
    Phát hiện bất thường từ dữ liệu telemetry ESP32.
    Input:  telemetry dict từ ESP32
    Output: trạng_thái (NORMAL/WARNING/CRITICAL), lý do
    """
    hien_tai   = telemetry.get("so_nguoi_hien_tai", 0)
    gioi_han_ad = telemetry.get("gioi_han_phong", 10)
    ly_do_gui  = telemetry.get("ly_do_gui", "periodic")
    battery    = telemetry.get("battery_level", 100.0)
    signal     = telemetry.get("network_signal", -50)

    # FIX: Tính lại từ số người THỰC TẾ, không dùng canh_bao_qua_tai từ ESP32
    # Trường đó bị set bởi lệnh BLOCK_ENTRY gây vòng lặp vô hạn
    phan_tram  = min(int(hien_tai * 100 / gioi_han_ad), 100) if gioi_han_ad > 0 else 0
    qua_tai    = (hien_tai >= gioi_han_ad)

    anomalies = []
    trang_thai = "NORMAL"

    # Kiểm tra quá tải
    if qua_tai or phan_tram >= 100:
        trang_thai = "CRITICAL"
        anomalies.append(f"Phòng quá tải: {hien_tai} người ({phan_tram}% công suất)")

    elif phan_tram >= 80:
        trang_thai = "WARNING"
        anomalies.append(f"Gần quá tải: {phan_tram}% công suất")

    # Kiểm tra sự kiện từ chối
    if ly_do_gui == "tu_choi_qua_tai":
        trang_thai = "CRITICAL"
        anomalies.append("Hệ thống đã từ chối người vào do quá tải")

    # Kiểm tra pin yếu (có thể bị phá hoại thiết bị)
    if battery < 20:
        if trang_thai == "NORMAL":
            trang_thai = "WARNING"
        anomalies.append(f"Pin ESP32 yếu: {battery:.0f}%")

    # Kiểm tra tín hiệu mạng kém
    if signal < -80:
        anomalies.append(f"Tín hiệu WiFi yếu: {signal} dBm")

    return {
        "model_name": "AnomalyDetection_v1",
        "trang_thai": trang_thai,
        "anomalies":  anomalies,
        "reason":     " | ".join(anomalies) if anomalies else "Hoạt động bình thường"
    }


# ============================================================
# MODULE 2: OCCUPANCY PREDICTION
# Dự báo xu hướng số người dựa trên giờ trong ngày và % hiện tại
# ============================================================
def run_occupancy_prediction(telemetry: dict) -> dict:
    """
    Dự báo số người và xu hướng trong 30 phút tới.
    Input:  telemetry dict
    Output: du_bao_30p, xu_huong (TANG/GIAM/ON_DINH)
    """
    hien_tai     = telemetry.get("so_nguoi_hien_tai", 0)
    gioi_han     = telemetry.get("gioi_han_phong", 10)
    ly_do_gui    = telemetry.get("ly_do_gui", "periodic")

    hour = datetime.now().hour

    # Dự báo đơn giản dựa trên giờ cao điểm
    # 7–9h: giờ vào → TĂNG mạnh
    # 11–13h: giờ nghỉ trưa → GIẢM
    # 17–19h: giờ tan → GIẢM mạnh
    # 20–6h: ngoài giờ → ít người
    if 7 <= hour <= 9:
        xu_huong = "TANG"
        delta = int(gioi_han * 0.2)  # Dự báo tăng thêm 20% giới hạn
    elif 11 <= hour <= 13:
        xu_huong = "GIAM"
        delta = -int(hien_tai * 0.3)
    elif 17 <= hour <= 19:
        xu_huong = "GIAM"
        delta = -int(hien_tai * 0.4)
    elif 20 <= hour or hour <= 6:
        xu_huong = "ON_DINH"
        delta = 0
    else:
        xu_huong = "ON_DINH"
        delta = int(gioi_han * 0.05)

    # Nếu vừa có người vào liên tục → dự báo tăng tiếp
    if ly_do_gui == "nguoi_vao":
        delta = max(delta, 2)
        xu_huong = "TANG"

    du_bao_30p = max(0, min(hien_tai + delta, gioi_han + 5))

    return {
        "model_name": "OccupancyPrediction_v1",
        "du_bao_30p": du_bao_30p,
        "xu_huong":   xu_huong,
        "gio_phan_tich": hour
    }


# ============================================================
# RISK CLASSIFIER — kết hợp 2 module → mức rủi ro + hành động
# ============================================================
def classify_risk(telemetry: dict, anomaly: dict, prediction: dict) -> dict:
    hien_tai   = telemetry.get("so_nguoi_hien_tai", 0)
    gioi_han   = telemetry.get("gioi_han_phong", 10)
    phan_tram  = telemetry.get("phan_tram_su_dung", 0)
    trang_thai = anomaly.get("trang_thai", "NORMAL")
    du_bao     = prediction.get("du_bao_30p", 0)
    xu_huong   = prediction.get("xu_huong", "ON_DINH")

    risk_score = 0.0
    reasons    = [anomaly.get("reason", "")]

    if trang_thai == "CRITICAL":
        risk_score += 60
    elif trang_thai == "WARNING":
        risk_score += 30

    if du_bao >= gioi_han and xu_huong == "TANG":
        risk_score += 25
        reasons.append(f"Dự báo {du_bao} người trong 30 phút (giới hạn {gioi_han})")

    risk_score = min(risk_score, 100.0)

    if risk_score >= 60:
        risk_level = "HIGH"
        action = "BLOCK_ENTRY,SEND_ALERT,UPDATE_DASHBOARD"
    elif risk_score >= 30:
        risk_level = "MEDIUM"
        action = "WARN_USER,SEND_NOTIFICATION,UPDATE_DASHBOARD"
    elif risk_score >= 10:
        risk_level = "LOW"
        action = "LOG_EVENT,UPDATE_DASHBOARD"
    else:
        risk_level = "NONE"
        action = "LOG_ONLY"

    return {
        "risk_level":         risk_level,
        "risk_score":         round(risk_score, 1),
        "reason":             " | ".join(r for r in reasons if r),
        "recommended_action": action
    }


# ============================================================
# PIPELINE TỔNG HỢP
# ============================================================
def run_ai_pipeline(telemetry: dict) -> dict:
    anomaly    = run_anomaly_detection(telemetry)
    prediction = run_occupancy_prediction(telemetry)
    risk       = classify_risk(telemetry, anomaly, prediction)

    return {
        "anomaly":    anomaly,
        "prediction": prediction,
        "risk":       risk
    }
