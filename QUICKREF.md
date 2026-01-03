# Quick Reference Card - Cannabis Grow Monitor

## 🔌 Hardware Setup (Before Upload)

### Wiring SCD41 → ESP32
```
VIN  → 3V3
GND  → GND  
SCL  → GPIO 22
SDA  → GPIO 21
```

### When to Connect ESP32
**Connect ESP32 via USB-C NOW** before uploading code.

---

## ⚙️ Software Setup (One-Time)

### 1. Arduino IDE Libraries
Install via Library Manager:
- ✅ `Sensirion I2C SCD4x`
- ✅ `TaskScheduler`

### 2. WiFi Configuration
```bash
cp secrets.tmpl.h secrets.h
# Edit secrets.h with your WiFi SSID and password
```

### 3. Grow Stage Configuration
In `co2_sensor_scd4x_esp32_http_server.ino`:
```cpp
GrowStage currentGrowStage = GrowStage::VEG;     // or GrowStage::FLOWER
```

### 4. Upload
- Board: ESP32 Dev Module (or your board)
- Port: /dev/ttyUSB0 (or /dev/ttyACM0)
- Click Upload ➜

---

## 🌐 API Endpoints

### Prometheus Metrics (Default)
```bash
curl http://192.168.1.XXX/
# or
curl http://192.168.1.XXX/metrics
```

### JSON Status
```bash
curl http://192.168.1.XXX/status
```

Example response:
```json
{
  "temperature_c": 24.5,
  "humidity_percent": 55.3,
  "co2_ppm": 650,
  "vpd_kpa": 0.98,
  "vpd_status": "optimal",
  "grow_stage": "veg"
}
```

---

## 🌱 VPD Target Ranges

| Stage | VPD Range | Status |
|-------|-----------|--------|
| **VEG** | 0.8 - 1.2 kPa | `optimal` |
| **FLOWER** | 1.2 - 1.6 kPa | `optimal` |

**Classifications:**
- `too_low` - High humidity, mold risk
- `optimal` - Perfect for growth stage ✅
- `too_high` - Low humidity, plant stress

---

## 🔧 Common Tasks

### Change from Veg to Flower
1. Edit `.ino` file:
   ```cpp
   GrowStage currentGrowStage = GrowStage::FLOWER;
   ```
2. Re-upload to ESP32

### Check Serial Output
- Open Serial Monitor
- Set baud: **115200**
- Look for IP address and VPD readings

### Disable BLE (Already Default)
Ensure this line exists:
```cpp
#define DISABLE_BLE  // Reduces memory usage
```

### Adjust VPD Ranges
Edit `vpd.h`:
```cpp
case GrowStage::VEG:
    range.min_kPa = 0.8;  // Customize here
    range.max_kPa = 1.2;
```

---

## 🚨 Troubleshooting

| Problem | Solution |
|---------|----------|
| No WiFi | Check 2.4GHz network, verify secrets.h |
| Sensor errors | Check I²C wiring, wait 6 sec for first read |
| Can't access web | Verify IP from Serial Monitor, ping ESP32 |
| Garbled serial | Set baud to 115200 |
| BLE compile errors | Ensure `#define DISABLE_BLE` is present |

---

## 📊 Grafana Queries

**VPD Chart:**
```promql
vpd_kpa
```

**VPD Alert (Too High):**
```promql
vpd_status{status="too_high"} == 1
```

**CO₂ Levels:**
```promql
co2
```

---

## 📁 Project Files

```
├── co2_sensor_scd4x_esp32_http_server.ino  ← Main sketch
├── vpd.h                                    ← VPD calculations
├── secrets.h                                ← WiFi config (create this)
├── secrets.tmpl.h                           ← WiFi template
├── README_CANNABIS.md                       ← Full documentation
└── QUICKREF.md                              ← This file
```

---

## 📖 Full Documentation

See **README_CANNABIS.md** for:
- Detailed VPD formulas
- Grafana integration
- Power optimization
- Safety guidelines
- Advanced configuration

---

**Happy Growing! 🌿**
