# connect_database.py — Kết nối SQL Server
import pyodbc
import os
from dotenv import load_dotenv

load_dotenv()

SQL_SERVER   = os.getenv("SQL_SERVER",   "localhost")
SQL_DATABASE = os.getenv("SQL_DATABASE", "PeopleCounter")
SQL_DRIVER   = os.getenv("SQL_DRIVER",   "ODBC Driver 17 for SQL Server")

# Windows Authentication (không cần user/pass)
CONNECTION_STRING = (
    f"DRIVER={{{SQL_DRIVER}}};"
    f"SERVER={SQL_SERVER};"
    f"DATABASE={SQL_DATABASE};"
    f"Trusted_Connection=yes;"
)

def get_connection():
    return pyodbc.connect(CONNECTION_STRING, timeout=5)

def test_connection():
    try:
        conn = get_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT @@VERSION")
        row = cursor.fetchone()
        conn.close()
        print(f"[DB OK] {row[0][:60]}...")
        return True
    except Exception as e:
        print(f"[DB FAIL] {e}")
        return False

if __name__ == "__main__":
    test_connection()
