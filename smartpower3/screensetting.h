#include <Arduino.h>
#include <TFT_eSPI.h>
#include "component.h"
#include "fonts/NotoSansBold20.h"
#include "settings.h"

#define BL_LCD	17
#define FREQ	50000
#define RESOLUTION	8

#define X_BL_LEVEL 250
#define Y_BL_LEVEL 20

#define Y_LOGGING 60
#define Y_PADDING_LOG 30

#define Y_SERIAL 60
#define X_BAUD_RATE 225

#define X_LOG_LEVEL 250
#define Y_WIFI_INFO	140
#define X_IPADDR 220
#define X_SSID 180

#define COLOR_SELECTED TFT_YELLOW
#define COLOR_DESELECTED TFT_BLACK

class ScreenSetting
{
public:
	ScreenSetting(TFT_eSPI *tft, Settings *settings);
	void init(uint16_t x, uint16_t y);

	uint8_t setBacklightLevelPreset(void);
	void setBacklightLevelPreset(uint8_t level_preset, bool edit);
	void setBacklightLevel(uint8_t level);
	void turnOffBacklight(void);
	uint8_t setLogInterval(void);
	void setLogInterval(uint8_t val);
	bool isLoggingEnabled(void);
	void enableLogging(void);
	void disableLogging(void);
	uint32_t setSerialBaud(uint32_t baud);
	uint32_t setSerialBaud();

	uint8_t getBacklightLevel(void);
	uint8_t getLogInterval(void);
	uint16_t getLogIntervalValue(void);
	uint32_t getSerialBaud(void);
	uint8_t getSerialBaudIndex(void);

	void changeBacklight(uint8_t level=255);
	void restoreBacklight();
	void changeLogInterval(uint8_t log_interval);
	void changeSerialBaud(uint8_t baud_level);
	void restoreSerialBaud(void);
	void restoreLogIntervalValue(void);

	void selectBLLevel(uint16_t color=COLOR_SELECTED);
	void selectLogInterval(uint16_t color=COLOR_SELECTED);
	void selectSerialBaud(uint16_t color=COLOR_SELECTED);
	void selectSerialLogging(uint16_t color=COLOR_SELECTED);

	void deSelectBLLevel(uint16_t color=COLOR_DESELECTED);
	void deSelectLogInterval(uint16_t color=COLOR_DESELECTED);
	void deSelectSerialBaud(uint16_t color=COLOR_DESELECTED);
	void deSelectSerialLogging(uint16_t color=COLOR_DESELECTED);

	void drawBacklightLevel(uint8_t level);
	void drawLogIntervalValue(uint16_t log_interval);
	void drawSerialBaud(uint32_t serial_baud);
	void popUp(void);

	void drawIpaddr(String ipaddr);
	void drawUDPIpaddr(String ipaddr);
	void drawUDPport(uint16_t port);
	void drawSSID(String ssid);

	void debug(void);
private:
	TFT_eSPI *tft;
	TFT_eSprite *popup;
	Settings *settings;
	uint16_t x;
	uint16_t y;
	uint8_t backlight_level_preset = 0;
	uint8_t backlight_level_edit = 0;
	uint16_t log_interval = 0;
	uint16_t log_interval_edit = 0;
	uint8_t bl_value_preset[7] = {10, 25, 50, 75, 100, 125, 150};
	uint16_t log_value[7] = {0, 5, 10, 50, 100, 500, 1000};
	uint32_t serial_value[10] = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 500000, 576000, 921600};
	uint32_t serial_baud = 0;
	uint32_t serial_baud_edit = 0;
	Component *com_serial_baud;
	Component *com_log_interval;
	Component *com_ipaddr;
	Component *com_ssid;
	Component *com_udp_ipaddr;
	Component *com_udp_port;
	uint8_t _setBacklightLevelPreset(uint8_t level_preset);
};
