# ============================================================
# app.py — FastAPI Backend People Counter AIoT
# Chạy: uvicorn app:app --host 0.0.0.0 --port 8000 --reload
# ============================================================
import sys, os
sys.path.insert(0, os.path.dirname(__file__))

from fastapi import FastAPI, HTTPException, Request
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from pydantic import BaseModel
from typing import Optional
from datetime import datetime

from connect_database import get_connection
from ai.ai_engine import run_ai_pipeline

app = FastAPI(title="People Counter AIoT", version="1.0.0")
app.mount("/static", StaticFiles(directory="static"), name="static")
templates = Jinja2Templates(directory="templates")

# SCHEMA
class TelemetryData(BaseModel):
    device_id:          str
    zone_id:            Optional[str]   = "zone_A_entrance"
    so_nguoi_hien_tai:  Optional[int]   = 0
    so_nguoi_vao:       Optional[int]   = 0
    so_nguoi_ra:        Optional[int]   = 0
    gioi_han_phong:     Optional[int]   = 10
    phan_tram_su_dung:  Optional[int]   = 0
    canh_bao_qua_tai:   Optional[bool]  = False
    che_do_chan:        Optional[bool]  = True
    cua_dang_mo:        Optional[bool]  = False
    battery_level:      Optional[float] = 100.0
    network_signal:     Optional[int]   = -50
    ly_do_gui:          Optional[str]   = "periodic"

class CommandData(BaseModel):
    device_id:    str
    command:      str
    payload:      Optional[str] = None
    triggered_by: Optional[str] = "MANUAL"

# DASHBOARD
@app.get("/", response_class=HTMLResponse)
async def dashboard(request: Request):
    return templates.TemplateResponse("dashboard.html", {"request": request})

@app.post("/api/telemetry")
async def receive_telemetry(data: TelemetryData):
    conn   = get_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            INSERT INTO telemetry
              (device_id, zone_id, so_nguoi_hien_tai, so_nguoi_vao, so_nguoi_ra,
               gioi_han_phong, phan_tram_su_dung, canh_bao_qua_tai, che_do_chan,
               cua_dang_mo, battery_level, network_signal, ly_do_gui, timestamp)
            OUTPUT INSERTED.id
            VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,GETDATE())
        """, (
            data.device_id, data.zone_id,
            data.so_nguoi_hien_tai, data.so_nguoi_vao, data.so_nguoi_ra,
            data.gioi_han_phong, data.phan_tram_su_dung,
            data.canh_bao_qua_tai, data.che_do_chan, data.cua_dang_mo,
            data.battery_level, data.network_signal, data.ly_do_gui
        ))
        telemetry_id = cursor.fetchone()[0]

        ai = run_ai_pipeline(data.dict())
        anomaly    = ai["anomaly"]
        prediction = ai["prediction"]
        risk       = ai["risk"]
        cursor.execute("""
            INSERT INTO ai_results
              (telemetry_id, device_id, model_name, trang_thai,
               du_bao_30p, xu_huong, risk_level, risk_score,
               reason, recommended_action, timestamp)
            VALUES (?,?,?,?,?,?,?,?,?,?,GETDATE())
        """, (
            telemetry_id, data.device_id,
            f"{anomaly['model_name']}+{prediction['model_name']}",
            anomaly["trang_thai"],
            prediction["du_bao_30p"], prediction["xu_huong"],
            risk["risk_level"], risk["risk_score"],
            risk["reason"], risk["recommended_action"]
        ))

        if risk["risk_level"] in ("MEDIUM", "HIGH"):
            event_type = "QUA_TAI" if data.canh_bao_qua_tai else "DU_BAO_QUA_TAI"
            cursor.execute("""
                INSERT INTO events (device_id, zone_id, event_type, risk_level, description)
                VALUES (?,?,?,?,?)
            """, (data.device_id, data.zone_id, event_type,
                  risk["risk_level"], risk["reason"]))

        auto_cmds = []

        if risk["risk_level"] == "HIGH" and data.so_nguoi_hien_tai > 0 and data.phan_tram_su_dung >= 80:
            cursor.execute("""
                SELECT COUNT(*) FROM commands
                WHERE device_id=? AND command='BLOCK_ENTRY' AND status='pending'
            """, (data.device_id,))
            existing = cursor.fetchone()[0]
            if existing == 0:
                cursor.execute("""
                    INSERT INTO commands (device_id, command, status, triggered_by)
                    VALUES (?,'BLOCK_ENTRY','pending','AI')
                """, (data.device_id,))
                auto_cmds.append("BLOCK_ENTRY")
        elif data.so_nguoi_hien_tai == 0 or risk["risk_level"] == "NONE":
            cursor.execute("""
                UPDATE commands SET status='cancelled'
                WHERE device_id=? AND command='BLOCK_ENTRY' AND status='pending'
            """, (data.device_id,))

        conn.commit()
        return {
            "status":       "ok",
            "telemetry_id": telemetry_id,
            "ai":           ai,
            "auto_commands": auto_cmds
        }
    except Exception as e:
        conn.rollback()
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        conn.close()

# API 2: SỰ KIỆN
@app.get("/api/events")
async def get_events(limit: int = 20):
    conn = get_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT TOP (?) id, device_id, zone_id, event_type,
                   risk_level, description, resolved, created_at
            FROM events ORDER BY created_at DESC
        """, (limit,))
        cols = [c[0] for c in cursor.description]
        rows = [dict(zip(cols, r)) for r in cursor.fetchall()]
        for r in rows:
            r["created_at"] = str(r["created_at"])
        return {"events": rows}
    finally:
        conn.close()

# API 3: DỮ LIỆU MỚI NHẤT
@app.get("/api/devices/{device_id}/latest")
async def get_latest(device_id: str):
    conn = get_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT TOP 1 t.*, a.trang_thai, a.du_bao_30p, a.xu_huong,
                   a.risk_level, a.risk_score, a.reason, a.recommended_action
            FROM telemetry t
            LEFT JOIN ai_results a ON a.telemetry_id = t.id
            WHERE t.device_id = ?
            ORDER BY t.timestamp DESC
        """, (device_id,))
        cols = [c[0] for c in cursor.description]
        row  = cursor.fetchone()
        if not row:
            raise HTTPException(status_code=404, detail="Không có dữ liệu")
        result = dict(zip(cols, row))
        result["timestamp"] = str(result["timestamp"])
        return result
    finally:
        conn.close()

# API 4: LỊCH SỬ (cho vẽ biểu đồ)
@app.get("/api/devices/{device_id}/history")
async def get_history(device_id: str, limit: int = 30):
    conn = get_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT TOP (?) so_nguoi_hien_tai, phan_tram_su_dung,
                   canh_bao_qua_tai, timestamp
            FROM telemetry WHERE device_id=?
            ORDER BY timestamp DESC
        """, (limit, device_id))
        cols = [c[0] for c in cursor.description]
        rows = [dict(zip(cols, r)) for r in cursor.fetchall()]
        for r in rows:
            r["timestamp"] = str(r["timestamp"])
        return {"history": list(reversed(rows))}
    finally:
        conn.close()

# API 5: TRẠNG THÁI HỆ THỐNG
@app.get("/api/status")
async def system_status():
    conn = get_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT COUNT(*) FROM devices WHERE status='active'")
        devices = cursor.fetchone()[0]
        cursor.execute("SELECT COUNT(*) FROM events WHERE resolved=0")
        events = cursor.fetchone()[0]
        cursor.execute("SELECT TOP 1 so_nguoi_hien_tai, phan_tram_su_dung, risk_level FROM telemetry t LEFT JOIN ai_results a ON a.telemetry_id=t.id ORDER BY t.timestamp DESC")
        row = cursor.fetchone()
        return {
            "status":            "online",
            "active_devices":    devices,
            "open_events":       events,
            "so_nguoi_hien_tai": row[0] if row else 0,
            "phan_tram_su_dung": row[1] if row else 0,
            "risk_level":        row[2] if row else "NONE",
            "timestamp":         str(datetime.now())
        }
    finally:
        conn.close()

# API 6: ĐIỀU KHIỂN THỦ CÔNG TỪ DASHBOARD
@app.post("/api/control")
async def send_command(data: CommandData):
    conn = get_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            INSERT INTO commands (device_id, command, payload, status, triggered_by)
            OUTPUT INSERTED.id VALUES (?,?,?,'pending',?)
        """, (data.device_id, data.command, data.payload, data.triggered_by))
        cmd_id = cursor.fetchone()[0]
        conn.commit()
        return {"status": "ok", "command_id": cmd_id}
    finally:
        conn.close()

# API 7: ESP32 LẤY LỆNH ĐANG CHỜ
@app.get("/api/commands/{device_id}/pending")
async def get_pending_command(device_id: str):
    conn = get_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT TOP 1 id, command, payload
            FROM commands
            WHERE device_id=? AND status='pending'
            ORDER BY created_at ASC
        """, (device_id,))
        row = cursor.fetchone()
        if not row:
            return {"id": 0, "command": "", "payload": None}
        return {"id": row[0], "command": row[1], "payload": row[2]}
    finally:
        conn.close()

# API 8: ESP32 BÁO ĐÃ THỰC THI LỆNH XONG
@app.post("/api/commands/{cmd_id}/done")
async def mark_command_done(cmd_id: int):
    conn = get_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            UPDATE commands
            SET status='executed', executed_at=GETDATE()
            WHERE id=?
        """, (cmd_id,))
        conn.commit()
        return {"status": "ok"}
    finally:
        conn.close()

# API 9: XÓA TẤT CẢ LỆNH PENDING (dùng khi hệ thống bị loop)
@app.post("/api/commands/clear")
async def clear_pending_commands(device_id: str = "people_counter_zone_A"):
    conn = get_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            UPDATE commands SET status='cancelled'
            WHERE device_id=? AND status='pending'
        """, (device_id,))
        affected = cursor.rowcount
        conn.commit()
        return {"status": "ok", "cancelled": affected, "message": f"Đã hủy {affected} lệnh pending"}
    finally:
        conn.close()

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("app:app", host="0.0.0.0", port=8000, reload=True)
