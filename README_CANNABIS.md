# Cannabis Grow Room Monitor - ESP32 + SCD41 + VPD

A cannabis-focused environmental monitoring system built on ESP32 with the Sensirion SCD41 sensor. Monitors CO₂, temperature, humidity, and **calculates VPD (Vapor Pressure Deficit)** with cannabis-specific target ranges for vegetative and flowering stages.

![Cannabis Grow Monitor](scd-41-co2-temperature-humidity.png)

## Features

- **CO₂ Monitoring**: Track CO₂ levels (ppm) critical for photosynthesis
- **Temperature & Humidity**: Monitor grow room environmental conditions
- **VPD Calculation**: Automatic vapor pressure deficit calculation optimized for cannabis
- **Cannabis Growth Stages**: Pre-configured VPD ranges for veg and flower
- **VPD Status Classification**: Real-time alerts for too_low, optimal, or too_high VPD
- **Prometheus Metrics**: Export data for Grafana dashboards and monitoring
- **JSON API**: Simple `/status` endpoint for custom web UIs or mobile apps
- **Optional BLE**: Can be disabled to reduce memory usage and complexity

## Cannabis VPD Ranges

The system uses industry-standard VPD ranges optimized for cannabis cultivation:

| Growth Stage | Target VPD Range | Description |
|--------------|------------------|-------------|
| **Vegetative** | 0.8 - 1.2 kPa | Promotes vigorous growth and transpiration |
| **Flowering** | 1.2 - 1.6 kPa | Encourages flower development, prevents mold |

**VPD Status Classification:**
- `too_low` - High humidity, risk of mold/mildew, slow transpiration
- `optimal` - Perfect conditions for current growth stage
- `too_high` - Low humidity, stress, nutrient uptake issues

## Hardware Requirements

### Required Components

- **ESP32 Development Board** (ESP32-U dev, ESP32-C3, ESP32-S3, etc.)
  - USB-C preferred for easy programming
  - Treated as generic `esp32dev` in Arduino IDE
- **Sensirion SCD41 CO₂ Sensor** (Adafruit breakout recommended)
  - I²C interface
  - Measures CO₂ (400-5000 ppm), Temperature (-10 to 60°C), Humidity (0-100% RH)

### Wiring (I²C Connection)

```
SCD41 Breakout → ESP32
--------------------------
VIN (3V3)       → 3V3
GND             → GND
SCL             → GPIO 22 (default I²C clock)
SDA             → GPIO 21 (default I²C data)
```

**Note**: If using ESP32-S3 QT Py, see pin definitions in code (GPIO 40/41).

### Optional Components

- LiPo battery (3.7V) for portable monitoring
- Enclosure suitable for grow room environment (consider humidity)

## Software Setup

### Prerequisites

- **Arduino IDE** 1.8.x or 2.x, OR
- **PlatformIO** (VS Code extension)
- Ubuntu or Linux development environment

### Arduino IDE Setup

1. **Install ESP32 Board Support**
   - Open Arduino IDE → File → Preferences
   - Add to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Tools → Board → Boards Manager → Search "ESP32" → Install

2. **Install Required Libraries**
   
   Via Arduino Library Manager (Sketch → Include Library → Manage Libraries):
   - `Sensirion I2C SCD4x` by Sensirion
   - `TaskScheduler` by Anatoli Arkhipenko
   
   **Optional** (only if you want BLE - currently disabled by default):
   ```bash
   cd ~/Documents/Arduino/libraries
   git clone https://github.com/Sensirion/arduino-ble-gadget.git
   git clone https://github.com/h2zero/NimBLE-Arduino.git
   ```

3. **Configure WiFi**
   ```bash
   cp secrets.tmpl.h secrets.h
   ```
   
   Edit `secrets.h` with your WiFi credentials:
   ```cpp
   char* SECRET_SSID = "YourWiFiSSID";
   char* SECRET_PASSWORD = "YourWiFiPassword";
   ```
   
   **Important**: ESP32 only supports 2.4 GHz WiFi networks.

4. **Configure Grow Stage**
   
   Open `co2_sensor_scd4x_esp32_http_server.ino` and locate:
   ```cpp
   // === CANNABIS GROW CONFIGURATION ===
   // Set to GrowStage::VEG or GrowStage::FLOWER
   GrowStage currentGrowStage = GrowStage::VEG;
   ```
   
   Change to `GrowStage::FLOWER` when your plants enter flowering.

5. **Select Sensor Type**
   
   Confirm the correct sensor is uncommented:
   ```cpp
   // #define USESCD30
   #define USESCD4X  // ← Should be uncommented for SCD41
   ```

6. **Optional: Enable/Disable BLE**
   
   By default, BLE is **disabled** to reduce complexity:
   ```cpp
   // Optional: Disable BLE to reduce complexity and memory usage
   // Comment out the next line to enable BLE
   #define DISABLE_BLE
   ```
   
   To enable BLE, comment out or delete `#define DISABLE_BLE`.

### Upload to ESP32

**⚡ WHEN TO CONNECT ESP32 VIA USB-C:**

**Connect your ESP32 now** via USB-C cable before uploading.

1. **Select Board**
   - Tools → Board → ESP32 Arduino → "ESP32 Dev Module" (or your specific board)
   
2. **Select Port**
   - Tools → Port → `/dev/ttyUSB0` or `/dev/ttyACM0` (will appear after connecting ESP32)
   - If you don't see a port, check:
     ```bash
     ls /dev/ttyUSB* /dev/ttyACM*
     ```
   - You may need to add yourself to the `dialout` group:
     ```bash
     sudo usermod -a -G dialout $USER
     # Log out and back in for this to take effect
     ```

3. **Upload**
   - Click the "Upload" button (right arrow) in Arduino IDE
   - Wait for "Done uploading" message
   - Open Serial Monitor (Tools → Serial Monitor, set to 115200 baud)

### Serial Monitor Output

You should see output like:

```
Connecting to YourWiFiSSID
.
WiFi connected.
IP address: 192.168.1.100
Waiting for first measurement... (5 sec)
Co2: 650
Temperature: 24.5
Humidity: 55.3
Voltage: 0.00
VPD: 0.98 kPa (optimal)

Co2: 652
Temperature: 24.6
Humidity: 55.1
VPD: 0.99 kPa (optimal)
```

**Note the IP address** - you'll use this to access the web endpoints.

## API Endpoints

Once connected to WiFi, the ESP32 exposes two HTTP endpoints:

### 1. Prometheus Metrics (Default) - `/` or `/metrics`

**URL**: `http://<ESP32_IP>/` or `http://<ESP32_IP>/metrics`

Returns Prometheus-formatted metrics for Grafana/monitoring:

```bash
curl http://192.168.1.100/
```

**Response**:
```prometheus
# HELP ambient_temperature Ambient temperature
# TYPE ambient_temperature gauge
ambient_temperature 24.50
# HELP temperature Measured temperature
# TYPE temperature gauge
temperature 24.50
# HELP ambient_humidity Ambient humidity
# TYPE ambient_humidity gauge
ambient_humidity 55.30
# HELP co2 CO2 concentration in ppm
# TYPE co2 gauge
co2 650
# HELP battery_voltage Battery voltage
# TYPE battery_voltage gauge
battery_voltage 0.00
# HELP vpd_kpa Vapor Pressure Deficit in kPa
# TYPE vpd_kpa gauge
vpd_kpa 0.98
# HELP vpd_status VPD status classification (-1=too_low, 0=optimal, 1=too_high)
# TYPE vpd_status gauge
vpd_status{stage="veg",status="optimal"} 0
```

### 2. JSON Status - `/status`

**URL**: `http://<ESP32_IP>/status`

Returns simple JSON for custom UIs, mobile apps, or JavaScript frontends:

```bash
curl http://192.168.1.100/status
```

**Response**:
```json
{
  "temperature_c": 24.5,
  "ambient_temperature_c": 24.5,
  "humidity_percent": 55.3,
  "co2_ppm": 650,
  "vpd_kpa": 0.98,
  "vpd_status": "optimal",
  "grow_stage": "veg",
  "battery_voltage": 0.0
}
```

## VPD Calculation Details

The system uses standard atmospheric VPD formulas:

### Formulas

**Saturation Vapor Pressure (es)** in kPa:
```
es = 0.6108 × exp((17.27 × T) / (T + 237.3))
```

**Actual Vapor Pressure (ea)** in kPa:
```
ea = es × (RH / 100)
```

**Vapor Pressure Deficit (VPD)** in kPa:
```
VPD = es - ea
```

Where:
- T = Temperature in °C
- RH = Relative Humidity in %

### Example Calculation

Given: 25°C temperature, 60% RH

1. es = 0.6108 × exp((17.27 × 25) / (25 + 237.3)) = **3.17 kPa**
2. ea = 3.17 × (60 / 100) = **1.90 kPa**
3. VPD = 3.17 - 1.90 = **1.27 kPa**

Classification for VEG (0.8-1.2 kPa): **too_high** ⚠️  
Classification for FLOWER (1.2-1.6 kPa): **optimal** ✅

## Grafana Integration

### Add Prometheus Data Source

1. Add ESP32 as Prometheus target in `prometheus.yml`:
   ```yaml
   scrape_configs:
     - job_name: 'cannabis_grow_monitor'
       static_configs:
         - targets: ['192.168.1.100:80']
   ```

2. Restart Prometheus

3. In Grafana:
   - Configuration → Data Sources → Add Prometheus
   - Query examples:
     - `co2` - CO₂ levels over time
     - `vpd_kpa` - VPD trends
     - `vpd_status` - Status alerts (-1, 0, 1)

### Example Grafana Queries

**VPD Over Time with Status Bands**:
```promql
vpd_kpa
```
Add horizontal threshold lines at 0.8, 1.2, 1.6 kPa.

**VPD Status Alerts**:
```promql
vpd_status{status="too_high"}
```
Create alert when value = 1 for >10 minutes.

**Environmental Overview Panel**:
```
Temperature: ${temperature}°C
Humidity: ${ambient_humidity}%
CO₂: ${co2} ppm
VPD: ${vpd_kpa} kPa (${vpd_status})
```

## Changing Growth Stages

As your plants transition from vegetative to flowering (or vice versa):

### Method 1: Recompile (Persistent)

1. Edit `co2_sensor_scd4x_esp32_http_server.ino`:
   ```cpp
   GrowStage currentGrowStage = GrowStage::FLOWER;  // Changed from VEG
   ```

2. Re-upload to ESP32

### Method 2: Future Enhancement (Not Yet Implemented)

Consider adding:
- Web UI with stage toggle button
- MQTT command to change stage remotely
- Persistent storage (EEPROM/SPIFFS) to remember stage across reboots

## Troubleshooting

### ESP32 Not Connecting to WiFi

- Verify 2.4 GHz network (5 GHz not supported)
- Check `secrets.h` for typos in SSID/password
- Move ESP32 closer to router during testing
- Check Serial Monitor for connection attempts

### Sensor Reading Errors

- Verify I²C wiring (SDA/SCL not swapped)
- Check 3.3V power supply is stable
- SCD41 requires ~6 seconds for first measurement
- Try power cycling the ESP32

### VPD Always Shows "too_low" or "too_high"

- Verify temperature/humidity readings are accurate
- Check grow stage setting matches your plants
- Sensor may need time to stabilize (15-30 min)
- Ensure sensor is not in direct airflow or heat source

### Serial Monitor Shows Garbled Text

- Set baud rate to **115200** in Serial Monitor
- Some ESP32 boards show boot messages at different rates (ignore)

### Can't Access Web Endpoints

- Verify ESP32 IP from Serial Monitor output
- Ensure ESP32 and computer on same network
- Try `ping <ESP32_IP>` to test connectivity
- Check firewall settings

### Compilation Errors

**BLE-related errors**: Ensure `#define DISABLE_BLE` is present, or install BLE libraries.

**VPD undefined**: Verify `vpd.h` is in the same folder as `.ino` file.

**Wire/I2C errors**: Check ESP32 board definitions are installed correctly.

## File Structure

```
growcontroller/
├── co2_sensor_scd4x_esp32_http_server.ino  # Main sketch
├── vpd.h                                    # VPD calculation library
├── secrets.h                                # WiFi credentials (create from template)
├── secrets.tmpl.h                           # WiFi credentials template
├── README.md                                # Original project README
└── README_CANNABIS.md                       # This file
```

## Advanced Configuration

### Temperature Compensation

If your sensor reads higher than ambient (due to ESP32 heat):

```cpp
temperatureCompensation = 5;  // Subtract 5°C from reading
```

This affects `ambient_temperature` metric only.

### Custom VPD Ranges

Edit `vpd.h` to adjust ranges:

```cpp
case GrowStage::VEG:
    range.min_kPa = 0.7;  // Lower min for humid climates
    range.max_kPa = 1.1;
    break;
```

### LED Behavior

- **Red blink**: Setup starting
- **Blue slow blink**: Connecting to WiFi
- **Green blink**: WiFi connected
- **Green pulse**: HTTP request received
- **Blue pulse**: BLE reading sent (if enabled)

Disable LED pulses by removing/commenting LED code in `readSensorCallback()` and HTTP handler.

## Power Consumption

- **WiFi active**: ~120-150 mA (depends on TX power)
- **Deep sleep** (not implemented): ~10 µA
- **Battery life estimate**: 3.7V 2000mAh battery → ~13-16 hours continuous

To reduce power, consider:
- Lowering WiFi TX power: `WiFi.setTxPower(WIFI_POWER_2dBm);`
- Increasing sensor read interval (currently 5 seconds)
- Implementing deep sleep between readings

## Resources & References

### Cannabis VPD Guides
- [Quest Dehumidifiers VPD Chart](https://www.questclimate.com/vpd-chart-vapor-pressure-deficit/)
- [Pulse VPD Guide](https://www.pulsegrow.com/blog/vpd)

### Related Projects
- [Original SCD4X Project](https://github.com/sighmon/co2_sensor_scd4x_esp32_http_server)
- [Prometheus/Grafana for Raspberry Pi](https://github.com/sighmon/prometheus-grafana-raspberry-pi)

### Hardware
- [Adafruit SCD-41](https://www.adafruit.com/product/5190)
- [Sensirion SCD4X Datasheet](https://sensirion.com/products/catalog/SCD41/)

## Contributing

Suggestions for improvements:
- Web UI for changing grow stage without recompiling
- Historical data logging to SD card
- Multiple sensor support (distributed monitoring)
- MQTT integration for Home Assistant
- Leaf temperature sensor for leaf VPD calculation

## License

- Original ESP32/SCD4X code: See [LICENSE](LICENSE)
- VPD additions: MIT License
- Cannabis cultivation information: Educational purposes only, comply with local laws

## Safety & Legal Disclaimer

**This is an environmental monitoring tool only.** Cannabis cultivation may be illegal in your jurisdiction. Users are solely responsible for compliance with all applicable laws and regulations. The authors assume no liability for misuse of this project.

**Electrical Safety**: Use appropriate power supplies, avoid water exposure, and follow electrical safety standards for grow room environments.

---

## Quick Start Summary

1. ✅ Clone repository
2. ✅ Install Arduino IDE + ESP32 board support
3. ✅ Install `Sensirion I2C SCD4x` library
4. ✅ Copy `secrets.tmpl.h` → `secrets.h`, add WiFi credentials
5. ✅ Set `currentGrowStage` to `VEG` or `FLOWER`
6. ✅ **Connect ESP32 via USB-C**
7. ✅ Wire SCD41 sensor to ESP32 (I²C)
8. ✅ Upload sketch to ESP32
9. ✅ Open Serial Monitor (115200 baud), note IP address
10. ✅ Access `http://<IP>/status` or `http://<IP>/metrics`
11. ✅ Monitor your grow! 🌱

Happy growing! 🌿
