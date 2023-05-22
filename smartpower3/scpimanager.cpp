#include "scpimanager.h"


UserContext::UserContext(ScreenManager *screen_manager, MeasChannels *measuring_channels)
{
	this->screen_manager = screen_manager;
	this->measuring_channels = measuring_channels;
	this->settings = screen_manager->getSettings();
}

SCPIManager::SCPIManager(ScreenManager *screen_manager, MeasChannels *measuring_channels)
{
	this->user_context = new UserContext(screen_manager, measuring_channels);
	mac_address.reserve(18);
}

SCPIManager::~SCPIManager()
{
}

void SCPIManager::init(void)
{
	snprintf(build_date, 34, "Build date: %s %s", String(__DATE__).c_str(), String(__TIME__).c_str());
	SCPI_Init(
		&scpi_context,
		scpi_commands,
		&scpi_interface,
		scpi_units_def,
		SCPI_IDN1, SCPI_IDN2, this->getMacAddress(), this->getBuildDate(),
		scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
		scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE);

	this->scpi_context.user_context = this->user_context;

	Serial.end();
	Serial.begin(115200);
	while (!Serial); // wait for serial to finish initializing
	Serial.println("SCPI Interactive demo");
}

size_t SCPIManager::SCPI_Write(scpi_t *context, const char *data, size_t len)
{
	(void) context;
	Serial.write(data, len);
	return len;
}

scpi_result_t SCPIManager::SCPI_Flush(scpi_t *context)
{
	(void) context;
	Serial.flush();
	return SCPI_RES_OK;
}

int SCPIManager::SCPI_Error(scpi_t *context, int_fast16_t err)
{
	(void) context;
	Serial.print ("**ERROR: ");
	Serial.print (err);
	Serial.print (", \"");
	Serial.print (SCPI_ErrorTranslate (err));
	Serial.println ("\"");
	return 0;
}

scpi_result_t SCPIManager::SCPI_Control(scpi_t *context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val)
{
	(void) context;

	if (SCPI_CTRL_SRQ == ctrl) {
		Serial.print ("**SRQ: 0x");
		Serial.print (val, HEX);
		Serial.print ("(");
		Serial.print (val, DEC);
		Serial.println (")");
	} else {
		Serial.print ("**CTRL: ");
		Serial.print (val, HEX);
		Serial.print ("(");
		Serial.print (val, DEC);
		Serial.println (")");
	}
	return SCPI_RES_OK;
}

scpi_result_t SCPIManager::SCPI_Reset(scpi_t * context)
{
	(void) context;
	Serial.println("**Reset");
	return SCPI_RES_OK;
}

void SCPIManager::processInput(Stream &communication_interface)
{
	if (communication_interface.available () > 0) {
		incoming_byte = communication_interface.read ();
		SCPI_Input(&scpi_context, &incoming_byte, 1);
	}
}

scpi_result_t SCPIManager::SCPI_SystemCommTcpipControlQ(scpi_t *context)
{
	(void) context;
	return SCPI_RES_ERR;
}

scpi_result_t SCPIManager::My_CoreTstQ(scpi_t * context)
{
//TODO: test low_power for connected power supply
	SCPI_ResultInt32(context, 0);
	return SCPI_RES_OK;
}

scpi_result_t SCPIManager::DeviceCapability(scpi_t * context)
{
	SCPI_ResultMnemonic(context, "DCPSUPPLY&MEASURE&MULTIPLE");
	//Serial.print ("**SRQ: 0x");
	return SCPI_RES_OK;
}

scpi_result_t SCPIManager::Reset(scpi_t * context)
{
	// Return device to defined known state
	// Should be a safe state in the sense that connected devices are not damaged or destroyed
	// for example by setting high output voltage
	uint8_t *onoff = (static_cast<UserContext *>(context->user_context))->screen_manager->getOnOff();

	for (int idx = 0; idx < 2; idx++) {
		if (onoff[idx] == 1) {
			onoff[idx] = 2;
		}
	}
	//screen_manager.getVoltageScreen()->getChannel(0)->off();
	//screen_manager.getVoltageScreen()->getChannel(1)->off();
	//screen_manager.disablePower();

	return SCPI_RES_OK;
}

scpi_result_t SCPIManager::DMM_MeasureVoltageDcQ(scpi_t *context)
{
	UserContext *user_ctx = static_cast<UserContext *>(context->user_context);
	//Settings *settings = user_ctx->settings;
	MeasChannels *channels = user_ctx->measuring_channels;
	//ScreenManager *sm = user_ctx->screen_manager;

	scpi_number_t expected_value;
	scpi_number_t resolution;

	float base_result = (static_cast<float>(channels->getChannel(0)->V()))/1000;
	char bf[15];
	char cf[15];

	//Serial.println("meas:volt:dc"); /* debug command name */

	/* read first parameter if present */
	// if this one is not present, then it's useless checkoing for the second one
	if (!SCPI_ParamNumber(context, scpi_special_numbers_def, &expected_value, FALSE)) {
		/* do something, if parameter not present */
		SCPI_ResultFloat(context, base_result);
		return SCPI_RES_OK;
	}
	/* read second paraeter if present */
	if (!SCPI_ParamNumber(context, scpi_special_numbers_def, &resolution, FALSE)) {
		/* do something, if parameter not present */
		SCPI_ResultFloat(context, base_result);
		return SCPI_RES_OK;
	}

	//SCPI_NumberToStr(context, scpi_special_numbers_def, &expected_value, bf, 15);
	//Serial.printf("\texpected_value P1=%s\r\n", bf);

	//SCPI_NumberToStr(context, scpi_special_numbers_def, &resolution, cf, 15);
	//Serial.printf("\tresolution P2=%s\r\n", cf);

	if (expected_value.unit != SCPI_UNIT_NONE && expected_value.unit != SCPI_UNIT_VOLT)	{
		SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);
		return SCPI_RES_ERR;
	}
	if (resolution.unit == SCPI_UNIT_NONE
			|| resolution.unit == SCPI_UNIT_UNITLESS
			|| resolution.unit == SCPI_UNIT_VOLT
	) {
		// Serial.println(resolution.content.value);
		SCPI_ResultFloat(context, base_result/(resolution.content.value));
		return SCPI_RES_OK;
	} else {
		SCPI_ErrorPush (context, SCPI_ERROR_INVALID_SUFFIX);
		return SCPI_RES_ERR;
	}
	return SCPI_RES_ERR;
}

scpi_result_t SCPIManager::Output_TurnOnOff(scpi_t *context)
{
	scpi_bool_t onoff_parameter;
	uint8_t channel_number = 0;

	/* read first parameter if present */
	// this parameter is mandatory, so error out if not present
	if (!SCPI_ParamBool(context, &onoff_parameter, TRUE)) {
		return SCPI_RES_ERR;
	}

	uint8_t *onoff = (static_cast<UserContext *>(context->user_context))->screen_manager->getOnOff();

	// this is basically the same as button swithching th echannel on/off
	if (onoff[channel_number] == 1 && !onoff_parameter) {
		onoff[channel_number] = 2;
	} else if (onoff[channel_number] == 0 && onoff_parameter) {
		onoff[channel_number] = 3;
	}

	return SCPI_RES_OK;
}

scpi_result_t SCPIManager::Output_TurnOnOffQ(scpi_t *context)
{
	uint8_t channel_number = 0;
	scpi_bool_t channel_on;

	uint8_t *onoff = (static_cast<UserContext *>(context->user_context))->screen_manager->getOnOff();

	if (onoff[channel_number] == 1 || onoff[channel_number] == 3) {
		channel_on = 1;
	} else if (onoff[channel_number] == 0  || onoff[channel_number] == 2) {
		channel_on = 0;
	} else {
		return SCPI_RES_ERR;
	}

	SCPI_ResultBool(context, channel_on);
	return SCPI_RES_OK;
}

scpi_result_t SCPIManager::DMM_ConfigureVoltage(scpi_t *context)
{
	UserContext *user_ctx = static_cast<UserContext *>(context->user_context);
	Settings *settings = user_ctx->settings;
	scpi_number_t target_volts;

	// required to accept MIN and MAX

	if (!SCPI_ParamNumber(context, scpi_special_numbers_def, &target_volts, TRUE)) {
		return SCPI_RES_ERR;
	}

	if (target_volts.unit == SCPI_UNIT_NONE
			|| target_volts.unit == SCPI_UNIT_UNITLESS
			|| target_volts.unit == SCPI_UNIT_VOLT
	) {
		if (target_volts.special && target_volts.content.tag == SCPI_NUM_MIN) {
			Serial.println("got MIN");
			settings->setChannel0Voltage(3000);  // hardware allowed minimum
			Serial.println(settings->getChannel0Voltage());
			Serial.println(settings->getChannel0Voltage(true));
			return SCPI_RES_OK;
		} else if (target_volts.special && target_volts.content.tag == SCPI_NUM_MAX) {
			Serial.println("got MAX");
			settings->setChannel0Voltage(20000);  // hardware allowed maximum
			return SCPI_RES_OK;
		} else if (target_volts.content.value < 3.0 || target_volts.content.value > 20) {
			SCPI_ErrorPush(context, SCPI_ERROR_DATA_OUT_OF_RANGE);  // full error list define
			return SCPI_RES_ERR;
		} else {
			Serial.printf("\texpected_value P1=%f\r\n", target_volts.content.value);
			Serial.printf("\texpected_value P1=%d\r\n", static_cast<uint16_t>((target_volts.content.value)*1000));
			settings->setChannel0Voltage(static_cast<uint16_t>((target_volts.content.value)*1000));
			return SCPI_RES_OK;
		}
	} else {
		SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);
		return SCPI_RES_ERR;
	}
	return SCPI_RES_ERR;
}

const char* SCPIManager::getMacAddress()
{
	mac_address = WiFi.macAddress();
	return mac_address.c_str();
}

char* SCPIManager::getBuildDate()
{
	return this->build_date;
}

/*ScreenManager SCPIManager::getScreenManager(void)
{
	return this->screen_manager;
}*/
