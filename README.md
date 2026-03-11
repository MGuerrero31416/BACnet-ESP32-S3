# ESP32-S3 BACnet MS/TP WiFi Display

ESP32-S3 based BACnet/IP device with TFT display featuring 20 BACnet objects: 4 Analog Values, 4 Binary Values, 4 Analog Inputs, 4 Binary Inputs, and 4 Binary Outputs. Includes built-in PMS5003 air quality sensor for PM2.5/PM1.0/PM10 monitoring.

It can simultaneously connect the BACnet device through WiFi (BACnet/IP), WiFi to Ethernet bridge, and MS/TP (RS485 using a MAX485 module).

You can easily add extra BACnet objects and map them to ESP32 GPIO for analog and digital inputs/outputs.

## Features

- **BACnet/IP Protocol**: Full BACnet/IP stack implementation
- **BACnet MS/TP**: RS485 MS/TP support alongside BACnet/IP (dual stack)
- **Live Display**: Real-time monitoring of BACnet objects on 170x320 TFT display
- **20 BACnet Objects**:
  - 4 Analog Values (AV1-4) - read/write with COV and NVS persistence
  - 4 Binary Values (BV1-4) - read/write with COV and NVS persistence
  - 4 Analog Inputs (AI1-4) - sensor inputs with COV and NVS persistence
  - 4 Binary Inputs (BI1-4) - binary states with COV and NVS persistence
  - 4 Binary Outputs (BO1-4) - writable control outputs with COV and NVS persistence
- **Writable Metadata**: Object `Name` and `Description` are writable for AV/BV/AI/BI/BO
- **WiFi Connectivity**: ESP32 with built-in WiFi for BACnet/IP communication
- **Arduino Framework**: Leverages Arduino ecosystem for easy hardware control
- **Change of Value (COV)**: Implements BACnet COV notifications for efficient real-time updates
- **Persistent Storage**: Attribute values modifiable from BACnet supervisor are automatically saved to ESP32 non-volatile memory (NVS) for retention across power cycles
- **NVS Override**: When `USER_OVERRIDE_NVS_ON_FLASH=1`, NVS is erased on boot and all values reset to defaults
- **Centralized Configuration**: User settings are centralized in [main/User_Settings.c](main/User_Settings.c)
- **Air Quality Monitoring**: PMS5003 PM2.5/PM1.0/PM10 sensor with automatic BACnet integration

## Photos
![ESP32 NAE](docs/images/ESP32_BACnet.jpg)
![ESP32 Metasys](docs/images/ESP32_Metasys.jpg)
![YABE Tool](docs/images/YABE.png)

## Hardware Requirements

- **Microcontroller**: ESP32-S3
- **Display**: ST7796S SPI TFT (320x480 panel, rotation 3 used)
- **Display Connections**:
  - MOSI (SDA): GPIO 10
  - SCLK (SCL): GPIO 9
  - CS: GPIO 13
  - DC: GPIO 12
  - RST: GPIO 11
  - BL (Backlight): GPIO 14

## Hardware Components

### ST7796S TFT Display
- **Resolution**: 320x480 pixels
- **Interface**: SPI (4-wire)
- **Driver**: Adafruit ST7735 and ST7789 Library (using ST7796S driver)

### PMS5003 Air Quality Sensor
- **Model**: Plantower PMS5003
- **Communication**: UART (9600 baud, 8N1)
- **Connections**:
  - PMS5003 TX → ESP32 GPIO5
  - PMS5003 RX → ESP32 GPIO4
  - RST (Reset) → ESP32 GPIO6
  - Power: 5V (requires 5V supply, not 3.3V)
  - GND: ESP32 GND
- **Measurements**:
  - PM1.0 (atmospheric)
  - PM2.5 (atmospheric) → mapped to **Analog Value 1** in BACnet (configurable)
  - PM10 (atmospheric)
  - Particle counts (0.3µm - 10µm ranges)
- **BACnet Mapping**: By default, PM2.5 is written to AV1. You can select any sensor parameter (PM1.0, PM2.5, PM10, or particle counts) and map it to any Analog Value object (AV1-AV4) by modifying the pms5003_task in [main/main.c](main/main.c).
- **Update Frequency**: 2-second intervals
- **Response**: ~2 seconds to environmental changes
- **Features**:
  - Fast and responsive sensor readings
  - Automatic byte-swapping for big-endian protocol
  - Checksum validation on all frames
  - Sensor disconnect detection with BACnet error indication (-1 value)

### WiFi Connectivity
- Built-in ESP32 WiFi for BACnet/IP communication
- Configured via [main/User_Settings.c](main/User_Settings.c)
- Default values in [main/User_Settings.c](main/User_Settings.c) use placeholders (`YOUR_WIFI_SSID` / `YOUR_WIFI_PASSWORD`) and should be changed for your environment
- Static IP option in [main/User_Settings.c](main/User_Settings.c). Set `USER_WIFI_USE_STATIC_IP` to 1 or 0

### BACnet MS/TP (RS485)
- **Transceiver**: MAX485 or equivalent RS485 converter
- **UART**: UART2
- **Connections**:
  - RO (RX) → ESP32 GPIO17 (ESP32-S3 U1 TXD)
  - DI (TX) → ESP32 GPIO18 (ESP32-S3 U1 RXD)
  - DE/RE → ESP32 GPIO16
- **Baud Rate**: 38400 (default)
- **MS/TP Settings**: MAC 21, Max Master 127, Max Info Frames 80
- **Discovery**: Some controllers (e.g., NAE) require manual add on the MS/TP field bus

## GPIO Summary

| Pin     | Component   | Signal              | Definition |
|---------|-------------|---------------------|------------|
| GPIO 4  | PMS5003     | TX (sensor RX)      | [components/pms5003/pms5003.h](components/pms5003/pms5003.h)
| GPIO 5  | PMS5003     | RX (sensor TX)      | [components/pms5003/pms5003.h](components/pms5003/pms5003.h)
| GPIO 6  | PMS5003     | RST (Reset)         | [components/pms5003/pms5003.h](components/pms5003/pms5003.h)
| GPIO 9  | TFT Display | SCLK (SPI Clock)    | [main/display.cpp](main/display.cpp)
| GPIO 10 | TFT Display | MOSI SDA (SPI Data) | [main/display.cpp](main/display.cpp)
| GPIO 11 | TFT Display | RST (Reset)         | [main/display.cpp](main/display.cpp)
| GPIO 12 | TFT Display | DC (Data/Command)   | [main/display.cpp](main/display.cpp)
| GPIO 13 | TFT Display | CS (Chip Select)    | [main/display.cpp](main/display.cpp)
| GPIO 14 | TFT Display | BACKLIGHT           | [main/display.cpp](main/display.cpp)
| GPIO 16 | MAX485      | DE/RE               | [main/mstp_rs485.c](main/mstp_rs485.c)
| GPIO 17 | MAX485      | RO (RX)             | [main/mstp_rs485.c](main/mstp_rs485.c)
| GPIO 18 | MAX485      | DI (TX)             | [main/mstp_rs485.c](main/mstp_rs485.c)


## Build Requirements

- ESP-IDF v5.5.1
- Python 3.11+
- xtensa-esp-elf toolchain

## Building

```bash
cd c:\git\BACnet-ESP32-S3
idf.py build
```

## Flashing

```bash
idf.py flash -p COM3
```

Or use the provided build/flash tasks in VS Code.

## Monitoring Serial Output

```bash
idf.py monitor -p COM3
```

## Configuration

### Display Driver Settings

Display initialization and pin mapping are configured in [main/display.cpp](main/display.cpp), including:

- `SPI.begin(9, -1, 10, 13)`
- `tft.init(320, 480, 0, 0, ST7796S_BGR)`
- `tft.invertDisplay(true)`
- `tft.setRotation(3)`

### FreeRTOS Configuration

Arduino framework requires FreeRTOS tick rate of 1000Hz. This is set in [sdkconfig](sdkconfig):

```
CONFIG_FREERTOS_HZ=1000
```

### User Settings (Centralized Configuration)

Most user-configurable settings are centralized in [main/User_Settings.c](main/User_Settings.c) and declared in [main/User_Settings.h](main/User_Settings.h), including:

- WiFi SSID/password and static IP settings
- BACnet Device Instance and BBMD registration
- BACnet/IP and MS/TP enable flags (`USER_ENABLE_BACNET_IP`, `USER_ENABLE_BACNET_MSTP`)
- MS/TP parameters (MAC, baud rate, max master, max info frames)
- Default object names, descriptions, units, and initial values

### BACnet Object Configuration

- **Analog Values (AV1-4)**: Configure names, descriptions, units, and initial values in [main/User_Settings.c](main/User_Settings.c)

- **Binary Values (BV1-4)**: Configure names, descriptions, active/inactive text, and initial states in [main/User_Settings.c](main/User_Settings.c)

- **Analog Inputs (AI1-4)**: Configure names, descriptions, units, and COV increments in [main/User_Settings.c](main/User_Settings.c). Read-only inputs suitable for sensor integration.

- **Binary Inputs (BI1-4)**: Configure names, descriptions, active/inactive text in [main/User_Settings.c](main/User_Settings.c). Read-only binary states.

- **Binary Outputs (BO1-4)**: Configure names, descriptions, active/inactive text, and initial states in [main/User_Settings.c](main/User_Settings.c). Writable control outputs with priority support.

### Sensor Data Mapping

- **PMS5003 Parameters**: Select which sensor parameter (PM1.0, PM2.5, PM10, or particle counts) to map to each Analog Value object in [main/main.c](main/main.c) - look for `pms5003_task()` function where sensor data is written to BACnet objects. Currently, PM2.5 atmospheric is written to AV1.

## Architecture

### Components

- **[components/bacnet-stack](components/bacnet-stack)** - BACnet/IP stack (modified from bacnet-stack/bacnet-stack)
- **[components/Adafruit_BusIO](components/Adafruit_BusIO)** - Adafruit BusIO support library
- **[components/Adafruit_GFX_Library](components/Adafruit_GFX_Library)** - Adafruit graphics primitives
- **[components/Adafruit_ST7735_and_ST7789_Library](components/Adafruit_ST7735_and_ST7789_Library)** - Adafruit ST77xx/ST7796S driver library
- **[main](main/)** - Application code
  - `main.c` - BACnet initialization and main loop
  - `analog_value.c/h` - Analog Value object creation and NVS persistence
  - `binary_value.c/h` - Binary Value object creation and NVS persistence
  - `analog_input.c/h` - Analog Input object creation and NVS persistence
  - `binary_input.c/h` - Binary Input object creation and NVS persistence
  - `binary_output.c/h` - Binary Output object creation and NVS persistence
  - `display.cpp` - TFT display driver
  - `wifi_helper.c` - WiFi configuration helpers

### Display Layout

| Item | Type | Display |
|------|------|---------|
| AV1 | Analog Value | Numeric (1 decimal) |
| AV2 | Analog Value | Numeric (1 decimal) |
| AV3 | Analog Value | Numeric (1 decimal) |
| AV4 | Analog Value | Numeric (1 decimal) |
| BV1 | Binary Value | ON/OFF + Status Dot (Blue=OFF, Green=ON) |
| BV2 | Binary Value | ON/OFF + Status Dot (Blue=OFF, Green=ON) |
| BV3 | Binary Value | ON/OFF + Status Dot (Blue=OFF, Green=ON) |
| BV4 | Binary Value | ON/OFF + Status Dot (Blue=OFF, Green=ON) |

## BACnet Integration

The device broadcasts its Device ID and manages BACnet objects that can be read/written by any BACnet/IP or BACnet MS/TP client (e.g., YABE, Tridium Niagara, Metasys).

### BACnet Objects Exposed

- **Device**: 31418 (configurable in [main/User_Settings.c](main/User_Settings.c))
- **Analog Values**: Instance 1, 2, 3, 4
- **Binary Values**: Instance 1, 2, 3, 4
- **Analog Inputs**: Instance 1, 2, 3, 4
- **Binary Inputs**: Instance 1, 2, 3, 4
- **Binary Outputs**: Instance 1, 2, 3, 4

## Modifications to bacnet-stack

This project uses the official [bacnet-stack](https://github.com/bacnet-stack/bacnet-stack) with the following modifications:

- **[components/bacnet-stack/](components/bacnet-stack/)** - Configured as ESP-IDF component
- Simplified for embedded systems (reduced features, optimized for ESP32)
- WiFi-based BACnet/IP instead of Ethernet

For a list of specific changes, see [BACNET_STACK_CHANGES.md](BACNET_STACK_CHANGES.md) (if available).

## Development Notes

### Display Boundary Constants

The display code uses boundary constants for easy layout modification:

```c
#define DISP_X0    0
#define DISP_Y0    0
#define DISP_X1    479
#define DISP_Y1    319
#define DISP_WIDTH 480
#define DISP_HEIGHT 320
```

Position all elements relative to these constants to avoid hardcoding coordinates.

## Troubleshooting

### Display orientation or color issues
If display output looks mirrored, rotated, or has swapped colors, adjust ST7796S init parameters and rotation in [main/display.cpp](main/display.cpp) and recompile.

### WiFi connection fails
Check SSID/password in [main/User_Settings.c](main/User_Settings.c), then verify WiFi init/connection flow in [main/wifi_helper.c](main/wifi_helper.c).

### Linker errors with Arduino
Ensure `CONFIG_FREERTOS_HZ=1000` is set in [sdkconfig](sdkconfig) and rebuild with `idf.py fullclean && idf.py build`.


## References

- [BACnet Stack GitHub](https://github.com/bacnet-stack/bacnet-stack)
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/)
- [Arduino-ESP32 GitHub](https://github.com/espressif/arduino-esp32)
- [Adafruit ST7735 and ST7789 Library GitHub](https://github.com/adafruit/Adafruit-ST7735-Library)


