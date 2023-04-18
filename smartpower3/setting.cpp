#include "setting.h"
#include "fonts/ChewyRegular24.h"
#include "fonts/ChewyRegular32.h"
#include "helpers.h"

Setting::Setting(TFT_eSPI *tft)
{
	this->tft = tft;

	ledcSetup(2, FREQ, RESOLUTION);
	ledcAttachPin(BL_LCD, 2);

	/*
	popup = new TFT_eSprite(tft);
	popup->createSprite(100, 100);
	*/

	com_serial_baud = new Component(tft, 115, 20, 2);
	com_log_interval = new Component(tft, 85, 20, 2);
	com_ssid = new Component(tft, 284, 22, 2);
	//com_ipaddr = new Component(tft, 220, 20, 2);
	com_udp_ipaddr = new Component(tft, 170, 20, 2);
	com_udp_port = new Component(tft, 60, 20, 2);
}


void Setting::init(uint16_t x, uint16_t y)
{
	this->x = x;
	this->y = y;

	tft->fillRect(0, 52, 480, 285, BG_COLOR);
	tft->loadFont(NotoSansBold20);
	tft->drawString("Build date : ", x + 140, y + 215, 2);
	tft->drawString(String(__DATE__), x + 260, y + 215, 2);
	tft->drawString(String(__TIME__), x + 380, y + 215, 2);
	tft->unloadFont();

	tft->loadFont(ChewyRegular32);
	tft->drawString("Backlight Level", x, y, 4);

	tft->drawString("Logging", x, y + Y_LOGGING, 4);

	tft->drawString("Serial", x + 120, y + Y_LOGGING, 4);

	tft->drawString("WiFi Info", x, y + Y_WIFI_INFO + 10, 4);
	tft->unloadFont();

	tft->fillRect(x + X_BL_LEVEL, y, 135, 26, BG_COLOR);
	tft->drawRect(x + X_BL_LEVEL, y-1, 135, 28, TFT_YELLOW);
	changeBacklight(backlight_level_preset);

	com_serial_baud->init(TFT_WHITE, BG_COLOR, 1, MC_DATUM);
	com_serial_baud->setCoordinate(x + X_BAUD_RATE-10, y + 30 + Y_SERIAL);
	com_log_interval->init(TFT_WHITE, BG_COLOR, 1, MC_DATUM);
	com_log_interval->setCoordinate(x + X_BAUD_RATE + 130, y + 30 + Y_SERIAL);

	com_ssid->init(TFT_WHITE, BG_COLOR, 1, MC_DATUM);
	com_ssid->setCoordinate(x + X_SSID, y + Y_WIFI_INFO);
	/*
	com_ipaddr->init(TFT_WHITE, BG_COLOR, 1, MC_DATUM);
	com_ipaddr->setCoordinate(x + X_IPADDR, y + Y_WIFI_INFO + 30);
	*/
	com_udp_ipaddr->init(TFT_WHITE, BG_COLOR, 1, MC_DATUM);
	com_udp_ipaddr->setCoordinate(x + X_IPADDR + 10, y + Y_WIFI_INFO + 30);
	com_udp_port->init(TFT_WHITE, BG_COLOR, 1, MC_DATUM);
	com_udp_port->setCoordinate(x + X_IPADDR + 175, y + Y_WIFI_INFO + 30);

	tft->loadFont(ChewyRegular24);
	tft->drawString("Baud Rate  /", x + X_BAUD_RATE, y + Y_SERIAL);
	tft->drawString("/", x + X_BAUD_RATE + 110, y + Y_SERIAL + 30);
	tft->drawString(" Interval", x + X_LOG_LEVEL+100, y + Y_SERIAL);
	tft->drawString("UDP : ", x + 180, y + Y_WIFI_INFO + 28, 4);
	tft->unloadFont();

	drawSerialBaud(this->serial_baud);
	drawLogIntervalValue(log_value[log_interval]);
}

void Setting::popUp(void)
{
	popup->fillSprite(TFT_DARKGREY);
	popup->setTextDatum(MC_DATUM);
	popup->drawString("Sprite", 200, 200, 4);
	popup->pushSprite(200, 200);
}

uint8_t Setting::_setBacklightLevelPreset(uint8_t level_preset)
{
	clampVariableToRange(0, 6, &level_preset);
	this->setBacklightLevel(bl_value_preset[level_preset]);
	return level_preset;
}

uint8_t Setting::setBacklightLevelPreset(void)
{
	backlight_level_preset = backlight_level_edit;
	return _setBacklightLevelPreset(backlight_level_preset);
}

void Setting::setBacklightLevelPreset(uint8_t level_preset, bool edit)
{
	backlight_level_preset = _setBacklightLevelPreset(level_preset);
	if (edit) {
		backlight_level_edit = backlight_level_preset;
	}
}

void Setting::setBacklightLevel(uint8_t level)
{
	clampVariableToRange(0, 255, &level);
	ledcWrite(2, level);
}

void Setting::turnOffBacklight(void)
{
	this->setBacklightLevel(0);
}

void Setting::setLogInterval(uint8_t val)
{
	log_interval = val;
}

bool Setting::isLoggingEnabled()
{
	return this->logging_enabled;
}

void Setting::enableLogging(void)
{
	logging_enabled = true;
}

void Setting::disableLogging(void)
{
	logging_enabled = false;
}

uint8_t Setting::setLogInterval(void)
{
	return log_interval = log_interval_edit;
}

uint32_t Setting::setSerialBaud()
{
	Serial.flush();
	Serial.begin(this->serial_baud_edit);
	return this->serial_baud = this->serial_baud_edit;
}

uint32_t Setting::setSerialBaud(uint32_t baud)
{
	Serial.flush();
	Serial.end();
	Serial.begin(baud);
	return serial_baud = baud;
}

uint8_t Setting::getBacklightLevel(void)
{
	return backlight_level_preset;
}

uint8_t Setting::getLogInterval(void)
{
	return log_interval;
}

uint16_t Setting::getLogIntervalValue(void)
{
	return log_value[log_interval];
}

uint32_t Setting::getSerialBaud(void)
{
	return serial_baud;
}

uint8_t Setting::getSerialBaudLevel(void)
{
	for (int i = 0; i < 10; i++) {
		if (serial_value[i] >= serial_baud) {
			serial_baud_edit = serial_baud;
			return i;
		}
	}
	return 0;
}

void Setting::restoreBacklight()
{
	backlight_level_edit = backlight_level_preset;
}

void Setting::changeBacklight(uint8_t level)
{
	if (level == 255) {
		level = backlight_level_preset;
	}
	drawBacklightLevel(level);
	backlight_level_edit = level;
	ledcWrite(2, bl_value_preset[level]);
}

void Setting::restoreLogIntervalValue()
{
	drawLogIntervalValue(log_value[this->log_interval]);
	log_interval_edit = this->log_interval;
}

void Setting::changeLogInterval(uint8_t log_interval)
{
	double ms = (1/(double)(this->serial_baud_edit/780))*1000;

	if (log_interval != 0) {
		clampVariableToRange(0, 6, &log_interval);
		while (log_value[log_interval] < ms) {
			log_interval++;
		}
	}
	drawLogIntervalValue(log_value[log_interval]);
	log_interval_edit = log_interval;
}

void Setting::restoreSerialBaud()
{
	drawSerialBaud(this->serial_baud);
	serial_baud_edit = this->serial_baud;
}

void Setting::changeSerialBaud(uint8_t baud_level)
{
	drawSerialBaud(serial_value[baud_level]);
	serial_baud_edit = serial_value[baud_level];
	changeLogInterval(log_interval);
}

void Setting::selectBLLevel(uint16_t color)
{
	for (int i = 1; i < SELECTION_BORDER_WIDTH+1; i++) {
		tft->drawRect(x + X_BL_LEVEL-i, y-1-i, 135+i*2, 28+i*2, color);
	}
}

void Setting::selectSerialLogging(uint16_t color)
{
	for (int i = 1; i < SELECTION_BORDER_WIDTH+1; i++) {
		tft->drawRect(x + X_LOG_LEVEL -i-50, y-1-i + Y_SERIAL -10, 135+i*2+120, 28+i*2+40, color);
	}
}

void Setting::selectLogInterval(uint16_t color)
{
	com_log_interval->setTextColor(color, BG_COLOR);
	com_log_interval->select();
}

void Setting::deSelectBLLevel(uint16_t color)
{
	for (int i = 1; i < SELECTION_BORDER_WIDTH+1; i++) {
		tft->drawRect(x + X_BL_LEVEL -i, y-1-i, 135+i*2, 28+i*2, color);
	}
}

void Setting::deSelectLogInterval(uint16_t color)
{
	com_log_interval->setTextColor(color, BG_COLOR);
	drawLogIntervalValue(log_value[log_interval]);
	com_log_interval->deSelect();
}

void Setting::deSelectSerialLogging(uint16_t color)
{
	for (int i = 1; i < SELECTION_BORDER_WIDTH+1; i++) {
		tft->drawRect(x + X_LOG_LEVEL -i-50, y-1-i + Y_SERIAL -10, 135+i*2+120, 28+i*2+40, color);
	}
}


void Setting::drawBacklightLevel(uint8_t level)
{
	tft->fillRect(x + X_BL_LEVEL + 2, y+1, 130, 24, BG_COLOR);
	for (int i = 0; i < level; i++) {
		tft->fillRect(x + X_BL_LEVEL + 2 + (i*22), y + 1, 20, 24, TFT_YELLOW);
	}
}

void Setting::selectSerialBaud(uint16_t color)
{
	com_serial_baud->setTextColor(color, BG_COLOR);
	drawSerialBaud(this->serial_baud);
	com_serial_baud->select();
}

void Setting::deSelectSerialBaud(uint16_t color)
{
	com_serial_baud->setTextColor(color, BG_COLOR);
	drawSerialBaud(this->serial_baud_edit);
	com_serial_baud->deSelect();
}

void Setting::drawLogIntervalValue(uint16_t log_value)
{
	com_log_interval->clearAndDrawWithFont(NotoSansBold20, (log_value == 0) ? "OFF" : String(log_value) + " ms");
}

void Setting::drawSerialBaud(uint32_t baud)
{
	com_serial_baud->clearAndDrawWithFont(NotoSansBold20, String(baud) + " bps");
}

void Setting::drawSSID(String ssid)
{
	com_ssid->clearAndDrawWithFont(NotoSansBold20, ssid);
}

void Setting::drawIpaddr(String ipaddr)
{
	com_ipaddr->clearAndDrawWithFont(NotoSansBold20, ipaddr);
}

void Setting::drawUDPIpaddr(String ipaddr)
{
	com_udp_ipaddr->clearAndDrawWithFont(NotoSansBold20, ipaddr);
}

void Setting::drawUDPport(uint16_t port)
{
	com_udp_port->clearAndDrawWithFont(NotoSansBold20, (" : " + String(port)));
}

void Setting::debug()
{
	com_serial_baud->clear();
}
