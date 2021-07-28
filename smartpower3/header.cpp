#include "header.h"

Header::Header(TFT_eSPI *tft)
{
	this->tft = tft;
	input = new Component(tft, 104, 30, 4);
	int0 = new Component(tft, 64, 22, 4);
}

void Header::init(uint16_t x, uint16_t y)
{
	this->x = x;
	this->y = y;
	input->init(TFT_BLACK, TFT_RED, 1, TL_DATUM);
	input->setCoordinate(x, y);
	input->draw("IN:0.0V");

	int0->init(FG_DISABLED, BG_DISABLED, 1, TL_DATUM);
	int0->setCoordinate(480 - 70, y);
	int0->draw("INT");
}

void Header::lowIntPin(void)
{
	if (intPin != 0) {
		intPin = 0;
		int0->setTextColor(FG_ENABLED, BG_ENABLED);
		int0->draw("INT");
	}
}

void Header::highIntPin(void)
{
	if (intPin != 1) {
		intPin = 1;
		int0->setTextColor(FG_DISABLED, BG_DISABLED);
		int0->draw("INT");
	}
}

void Header::activate(void)
{
	//save->activate();
}

void Header::deActivate(void)
{
	//save->deActivate();
}

void Header::drawMode(String str)
{
}

void Header::setLowInput(bool low_input)
{
	this->low_input = low_input;
}

bool Header::getLowInput(void)
{
	return low_input;
}

bool Header::checkInput(void)
{
	if (v_input > 10)
		return true;
	return false;
}

uint16_t Header::getInputVoltage(void)
{
	return v_input;
}

void Header::draw(void)
{
	if (v_update) {
		v_update = false;
		if (low_input)
			input->setTextColor(TFT_BLACK, TFT_RED);
		else
			input->setTextColor(TFT_BLACK, TFT_GREEN);
		input->clear();
		input->draw("IN:" + String(v_input/1000.0, 1) + "V");
	}
}

void Header::pushPower(uint16_t volt, uint16_t ampere, uint16_t watt)
{
	v_input = volt;
	v_update = true;
}
