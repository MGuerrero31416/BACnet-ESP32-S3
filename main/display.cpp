#include "display.h"
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S.h>
#include "User_Settings.h"
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/av.h"
#include "esp_netif.h"
#include "lwip/ip4_addr.h"
#include <stdio.h>
#include <string.h>

// Display wiring copied from DisplayTest (Adafruit ST7796S)
#define TFT_MOSI 10
#define TFT_SCLK 9
#define TFT_MISO -1
#define TFT_CS   13
#define TFT_DC   12
#define TFT_RST  11
#define TFT_BL   14

static Adafruit_ST7796S tft = Adafruit_ST7796S(TFT_CS, TFT_DC, TFT_RST);

// Rotation 3 produces 480x320 coordinate space on this panel.
#define DISP_X0    0
#define DISP_Y0    0
#define DISP_X1    479
#define DISP_Y1    319
#define DISP_WIDTH 480
#define DISP_HEIGHT 320

#define HEADER_HEIGHT   56
#define ROW_START_Y     66
#define ROW_SPACING     32
#define LABEL_X         8
#define VALUE_X         98
#define STATUS_DOT_X    190
#define UNIT_X          (STATUS_DOT_X + 5)
#define UNIT_WIDTH      120
#define HEADER_LED_R    7
#define HEADER_WIFI_X   376
#define HEADER_MSTP_X   376
#define HEADER_WIFI_TEXT_Y 8
#define HEADER_MSTP_TEXT_Y 30
#define HEADER_WIFI_LED_Y  (HEADER_WIFI_TEXT_Y + 8)
#define HEADER_MSTP_LED_Y  (HEADER_MSTP_TEXT_Y + 8)

static char s_last_ip_text[24] = "";
static uint32_t s_header_refresh_tick = 0;
static bool s_values_initialized = false;
static char s_last_av_text[4][32];
static int s_last_bv[4] = { -1, -1, -1, -1 };
static uint16_t s_last_av_units[4] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
static int s_wifi_connected = -1;
static int s_mstp_connected = -1;

static int row_y(int row) {
    return ROW_START_Y + (row * ROW_SPACING);
}

static const char *av_unit_text(uint16_t bacnet_unit) {
    switch (bacnet_unit) {
        case UNITS_MICROGRAMS_PER_CUBIC_METER:
            return "ug/m3";
        case UNITS_DEGREES_CELSIUS:
            return "C";
        case UNITS_PARTS_PER_MILLION:
            return "ppm";
        case UNITS_PERCENT:
            return "%";
        case UNITS_NO_UNITS:
            return "none";
        default:
            return NULL;
    }
}

static void draw_av_unit_line(int index, uint16_t unit)
{
    char unit_buf[16];
    const char *unit_text = av_unit_text(unit);
    if (!unit_text) {
        snprintf(unit_buf, sizeof(unit_buf), "u:%u", (unsigned)unit);
        unit_text = unit_buf;
    }

    tft.fillRect(UNIT_X, row_y(index), UNIT_WIDTH, 20, ST77XX_BLACK);
    tft.setCursor(UNIT_X, row_y(index));
    tft.print(unit_text);
}

static void get_ip_text(char *ip_text, size_t ip_text_size) {
    if (!ip_text || ip_text_size == 0) {
        return;
    }

    snprintf(ip_text, ip_text_size, "N/A");

    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif) {
        esp_netif_ip_info_t ip_info{};
        if (esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
            snprintf(ip_text, ip_text_size, IPSTR, IP2STR(&ip_info.ip));
        }
    }

    if (ip_text[0] == 'N' && USER_WIFI_USE_STATIC_IP) {
        snprintf(ip_text, ip_text_size, "%s", USER_WIFI_STATIC_IP_ADDR);
    }
}

static void draw_header(void) {
    char line[48];
    char ip_text[24] = "N/A";
    get_ip_text(ip_text, sizeof(ip_text));

    tft.fillRect(DISP_X0, DISP_Y0, DISP_WIDTH, HEADER_HEIGHT, ST77XX_BLUE);
    tft.drawFastHLine(DISP_X0, HEADER_HEIGHT, DISP_WIDTH, ST77XX_CYAN);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLUE);

    snprintf(line, sizeof(line), "BACnet ID: %lu", (unsigned long)USER_BACNET_DEVICE_INSTANCE);
    tft.setCursor(8, 8);
    tft.print(line);

    snprintf(line, sizeof(line), "IP: %s", ip_text);
    tft.setCursor(8, 30);
    tft.print(line);

    snprintf(s_last_ip_text, sizeof(s_last_ip_text), "%s", ip_text);
}

static void draw_link_indicators(bool force_redraw)
{
    if (!force_redraw && s_wifi_connected < 0 && s_mstp_connected < 0) {
        return;
    }

    static int last_wifi = -1;
    static int last_mstp = -1;
    if (!force_redraw && last_wifi == s_wifi_connected && last_mstp == s_mstp_connected) {
        return;
    }

    tft.fillRect(312, 2, 166, 52, ST77XX_BLUE);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLUE);

    tft.setCursor(314, HEADER_WIFI_TEXT_Y);
    tft.print("WiFi");
    tft.fillCircle(HEADER_WIFI_X, HEADER_WIFI_LED_Y, HEADER_LED_R,
                   s_wifi_connected > 0 ? ST77XX_GREEN : ST77XX_RED);

    tft.setCursor(314, HEADER_MSTP_TEXT_Y);
    tft.print("MSTP");
    tft.fillCircle(HEADER_MSTP_X, HEADER_MSTP_LED_Y, HEADER_LED_R,
                   s_mstp_connected > 0 ? ST77XX_GREEN : ST77XX_RED);

    last_wifi = s_wifi_connected;
    last_mstp = s_mstp_connected;
}

static void refresh_ip_line_if_changed(void) {
    char ip_text[24] = "N/A";
    char line[48];

    get_ip_text(ip_text, sizeof(ip_text));
    if (strcmp(ip_text, s_last_ip_text) == 0) {
        return;
    }

    tft.fillRect(8, 30, 300, 18, ST77XX_BLUE);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLUE);
    snprintf(line, sizeof(line), "IP: %s", ip_text);
    tft.setCursor(8, 30);
    tft.print(line);

    snprintf(s_last_ip_text, sizeof(s_last_ip_text), "%s", ip_text);
}

extern "C" void display_init(void) {
    // Initialize Arduino framework
    initArduino();
    
    // Turn on display backlight.
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    delay(10);
    
    // Initialize SPI + ST7796S with same startup sequence as DisplayTest.
    SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.init(320, 480, 0, 0, ST7796S_BGR);
    tft.invertDisplay(true);
    delay(1);
    tft.setRotation(3);
    tft.fillScreen(ST77XX_BLACK);
    delay(1);

    draw_header();
    draw_link_indicators(true);

    // Draw labels and static AV units.
    tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    tft.setTextSize(2);

    for (int i = 0; i < 4; i++) {
        char label[8];
        snprintf(label, sizeof(label), "AV%d:", i + 1);
        tft.setCursor(LABEL_X, row_y(i));
        tft.print(label);
        uint16_t unit = (uint16_t)Analog_Value_Units((uint32_t)(i + 1));
        draw_av_unit_line(i, unit);
        s_last_av_units[i] = unit;
    }

    for (int i = 0; i < 4; i++) {
        char label[8];
        snprintf(label, sizeof(label), "BV%d:", i + 1);
        tft.setCursor(LABEL_X, row_y(i + 4));
        tft.print(label);
    }
    
    printf("Display initialized\n");
}

extern "C" void display_set_link_status(bool wifi_connected, bool mstp_connected)
{
    s_wifi_connected = wifi_connected ? 1 : 0;
    s_mstp_connected = mstp_connected ? 1 : 0;
    draw_link_indicators(false);
}

extern "C" void display_update_values(float av1, float av2, float av3, float av4, int bv1, int bv2, int bv3, int bv4) {
    char buf[32];
    const float av_values[4] = { av1, av2, av3, av4 };
    const int bv_values[4] = { bv1, bv2, bv3, bv4 };

    // Refresh header IP occasionally and only if it changed to avoid flicker.
    s_header_refresh_tick++;
    if ((s_header_refresh_tick % 5U) == 0U) {
        refresh_ip_line_if_changed();
        draw_link_indicators(false);
    }

    // Update values
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setTextSize(2);

    for (int i = 0; i < 4; i++) {
        snprintf(buf, sizeof(buf), "%.1f", av_values[i]);
        if (!s_values_initialized || strcmp(buf, s_last_av_text[i]) != 0) {
            tft.fillRect(VALUE_X, row_y(i), 96, 20, ST77XX_BLACK);
            tft.setCursor(VALUE_X, row_y(i));
            tft.print(buf);
            snprintf(s_last_av_text[i], sizeof(s_last_av_text[i]), "%s", buf);
        }

        uint16_t unit = (uint16_t)Analog_Value_Units((uint32_t)(i + 1));
        if (!s_values_initialized || unit != s_last_av_units[i]) {
            tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
            tft.setTextSize(2);
            draw_av_unit_line(i, unit);
            s_last_av_units[i] = unit;
            tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
            tft.setTextSize(2);
        }
    }

    for (int i = 0; i < 4; i++) {
        if (!s_values_initialized || bv_values[i] != s_last_bv[i]) {
            tft.fillRect(VALUE_X, row_y(i + 4), 110, 20, ST77XX_BLACK);
            tft.setCursor(VALUE_X, row_y(i + 4));
            tft.print(bv_values[i] ? "ON " : "OFF");
            tft.fillCircle(STATUS_DOT_X, row_y(i + 4) + 8, 8,
                           bv_values[i] ? ST77XX_GREEN : ST77XX_BLUE);
            s_last_bv[i] = bv_values[i];
        }
    }

    s_values_initialized = true;
}
