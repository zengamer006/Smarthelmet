import json
import threading
import time
from datetime import datetime
from flask import Flask, jsonify, render_template
import serial
from serial.tools import list_ports

# Change COM19 if Windows assigns a different port.
SERIAL_PORT = "COM19"
BAUD_RATE = 115200

app = Flask(__name__)
lock = threading.Lock()
state = {
    "connected": False,
    "port": SERIAL_PORT,
    "baud": BAUD_RATE,
    "last_update": None,
    "latest": {},
    "raw": "",
    "history": [],
    "error": "Waiting for serial data..."
}

def try_parse_line(line):
    """Accept JSON lines first; otherwise parse common 'key=value' serial logs."""
    line = line.strip()
    if not line:
        return None
    try:
        obj = json.loads(line)
        if isinstance(obj, dict):
            return obj
    except json.JSONDecodeError:
        pass

    # Parse logs like: Hall=DETECTED | IR=NOT DETECTED | MQ3 raw=1234 | ADC V=0.99
    result = {}
    for part in line.split("|"):
        part = part.strip()
        if "=" in part:
            key, value = part.split("=", 1)
            result[key.strip()] = value.strip()
    return result or {"serialMessage": line}

def serial_worker():
    while True:
        try:
            with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
                # Many ESP32 boards reset when the serial port opens.
                time.sleep(2)
                with lock:
                    state["connected"] = True
                    state["error"] = ""
                while True:
                    raw_bytes = ser.readline()
                    if not raw_bytes:
                        continue
                    line = raw_bytes.decode("utf-8", errors="replace").strip()
                    if not line:
                        continue
                    parsed = try_parse_line(line)
                    with lock:
                        state["raw"] = line
                        state["latest"] = parsed or {"serialMessage": line}
                        state["last_update"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                        state["connected"] = True
                        state["error"] = ""
                        state["history"].append({
                            "time": datetime.now().strftime("%H:%M:%S"),
                            "data": state["latest"]
                        })
                        state["history"] = state["history"][-100:]
        except Exception as exc:
            with lock:
                state["connected"] = False
                state["error"] = str(exc)
            time.sleep(2)

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/api/data")
def api_data():
    with lock:
        return jsonify(state.copy())

@app.route("/api/ports")
def api_ports():
    return jsonify([{"device": p.device, "description": p.description} for p in list_ports.comports()])

if __name__ == "__main__":
    threading.Thread(target=serial_worker, daemon=True).start()
    # Local PC dashboard: http://127.0.0.1:5000
    app.run(host="127.0.0.1", port=5000, debug=False, threaded=True)
