ESP-NOW LOCAL SERIAL DASHBOARD (Windows)
========================================

WHAT THIS DOES
- ESP32 ESP-NOW receiver/root sends received sensor data to the USB serial port.
- Python reads COM19 at 115200 baud.
- A local website dashboard displays the newest line and fields.
- No Vercel, Firebase, internet connection, or cloud credentials are needed.

IMPORTANT
1. Close Arduino IDE Serial Monitor and Serial Plotter before launching this app.
   COM19 can normally be opened by only one program at a time.
2. Ensure the ESP32 receiver/root is the board assigned to COM19.
3. The receiver sketch must print data using Serial.println(...).
4. For best dashboard field matching, print one JSON object per line, e.g.:
   {"hallDetected":true,"irDetected":true,"mq3Raw":1200,"mq3Voltage":0.967,"greenLed":true,"redLed":false,"buzzerOn":false,"relayOn":true}
   The app also displays/parses common pipe-separated key=value serial logs.
5. Check the ESP32's actual baud rate. This app defaults to 115200.

SETUP
1. Install Python 3 from https://www.python.org/downloads/ if not installed.
   During installation, enable "Add Python to PATH".
2. Extract this ZIP into a normal folder, e.g. Desktop\espnow_local_dashboard.
3. Open Command Prompt in that folder.
4. Run:
      py -m venv .venv
      .venv\Scripts\activate
      python -m pip install -r requirements.txt
      python app.py
5. Open this in your browser:
      http://127.0.0.1:5000

IF COM19 IS WRONG / BUSY
- Edit SERIAL_PORT = "COM19" near the top of app.py.
- In Arduino IDE, Tools > Port can help identify the receiver port.
- Close Arduino IDE Serial Monitor.
- If you see "Access is denied" or "PermissionError", another program is using the port.
- If you see "could not open port", check the cable, driver, board, and COM number.

ESP-NOW ROOT CODE REQUIREMENT
The receiver/root code should print a single JSON line whenever it receives a packet. Example:
Serial.println("{\"hallDetected\":true,\"irDetected\":false,\"mq3Raw\":1234,\"mq3Voltage\":0.995,\"greenLed\":false,\"redLed\":true,\"buzzerOn\":false,\"relayOn\":false}");

If your existing ESP-NOW packet is a C/C++ struct, do not print the raw struct bytes as text. Print its actual fields into JSON. The dashboard parser will show unknown JSON fields in the "All latest fields" table too.

SAFETY
- MQ-3 raw ADC values are not calibrated alcohol concentration.
- Never feed more than 3.3 V into ESP32 GPIO inputs.
- Test relay outputs with a safe mock load, not a real vehicle ignition.
- This server binds to 127.0.0.1, so it is accessible only from this computer by default.
