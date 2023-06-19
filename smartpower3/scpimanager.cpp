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
	snprintf(build_date, 34, F("Build date: %s %s"), F(__DATE__), F(__TIME__));
	SCPI_Init(
		&scpi_context,
		scpi_commands,
		&scpi_interface,
		scpi_units_def,
		//SCPI_IDN1, SCPI_IDN2, this->user_context->settings->getMacAddress().c_str(), this->getBuildDate(),
		SCPI_IDN1, SCPI_IDN2, this->getMacAddress(), this->getBuildDate(),
		scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
		scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE);

	this->scpi_context.user_context = this->user_context;

	Serial.end();
	Serial.begin(115200);
	while (!Serial); // wait for serial to finish initializing
	Serial.println(F("SCPI Interactive demo"));
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
	Serial.print(F("**ERROR: "));
	Serial.print(err);
	Serial.print(F(", \""));
	Serial.print(SCPI_ErrorTranslate (err));
	Serial.println(F("\""));
	return 0;
}

scpi_result_t SCPIManager::SCPI_Control(scpi_t *context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val)
{
	(void) context;

	if (SCPI_CTRL_SRQ == ctrl) {
		Serial.print(F("**SRQ: 0x"));
		Serial.print(val, HEX);
		Serial.print(F("("));
		Serial.print(val, DEC);
		Serial.println(F(")"));
	} else {
		Serial.print(F("**CTRL: "));
		Serial.print(val, HEX);
		Serial.print(F("("));
		Serial.print(val, DEC);
		Serial.println(F(")"));
	}
	return SCPI_RES_OK;
}

scpi_result_t SCPIManager::SCPI_Reset(scpi_t * context)
{
	(void) context;
	Serial.println(F("**Reset"));
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

	float base_result = (static_cast<float>(channels->getChannel(1)->V()))/1000;

	/* read first parameter if present */
	// if this one is not present, then it's useless checkoing for the second one
	if (!SCPI_ParamNumber(context, scpi_special_numbers_def, &expected_value, FALSE)) {
		SCPI_ResultFloat(context, base_result);
		return SCPI_RES_OK;
	}
	if (!SCPI_ParamNumber(context, scpi_special_numbers_def, &resolution, FALSE)) {
		SCPI_ResultFloat(context, base_result);
		return SCPI_RES_OK;
	}

	if (expected_value.unit != SCPI_UNIT_NONE && expected_value.unit != SCPI_UNIT_VOLT)	{
		SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);
		return SCPI_RES_ERR;
	}
	if (resolution.unit == SCPI_UNIT_NONE
			|| resolution.unit == SCPI_UNIT_UNITLESS
			|| resolution.unit == SCPI_UNIT_VOLT
	) {
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
	int32_t output_number[1];

	if (!SCPI_CommandNumbers(context, output_number, 1, 1)) {
		return SCPI_RES_ERR;
	}
	// this parameter is mandatory, so error out if not present
	if (!SCPI_ParamBool(context, &onoff_parameter, TRUE)) {
		return SCPI_RES_ERR;
	}
	if (output_number[0] < 1 || output_number[0] > 2) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	channel_number = output_number[0] - 1;
	uint8_t *onoff = (static_cast<UserContext *>(context->user_context))->screen_manager->getOnOff();

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
	int32_t output_number[1];

	if (!SCPI_CommandNumbers(context, output_number, 1, 1)) {
		return SCPI_RES_ERR;
	}
	if (output_number[0] < 1 || output_number[0] > 2) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	channel_number = output_number[0] - 1;
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
	int32_t output_number[1];

	if (!SCPI_CommandNumbers(context, output_number, 1, 1)) {
		return SCPI_RES_ERR;
	}
	// required to accept MIN and MAX
	if (!SCPI_ParamNumber(context, scpi_special_numbers_def, &target_volts, TRUE)) {
		return SCPI_RES_ERR;
	}
	if (output_number[0] < 1 || output_number[0] > 2) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (target_volts.unit == SCPI_UNIT_NONE
			|| target_volts.unit == SCPI_UNIT_UNITLESS
			|| target_volts.unit == SCPI_UNIT_VOLT
	) {
		if (target_volts.special && target_volts.content.tag == SCPI_NUM_MIN) {
			if (output_number[0] == 1) {
				settings->setChannel0Voltage(3000, true);  // hardware allowed minimum
			} else if (output_number[0] == 2) {
				settings->setChannel1Voltage(3000, true);  // hardware allowed minimu
			}
			return SCPI_RES_OK;
		} else if (target_volts.special && target_volts.content.tag == SCPI_NUM_MAX) {
			if (output_number[0] == 1) {
				settings->setChannel0Voltage(20000, true);  // hardware allowed maximum
			} else if (output_number[0] == 2) {
				settings->setChannel1Voltage(20000, true);  // hardware allowed maximum
			}
			return SCPI_RES_OK;
		} else if (target_volts.content.value < 3.0 || target_volts.content.value > 20) {
			//SCPI_ErrorPush(context, SCPI_ERROR_DATA_OUT_OF_RANGE);  // full error list define
			SCPI_ErrorPush(context, SCPI_ERROR_DATA_TYPE_ERROR);
			return SCPI_RES_ERR;
		} else {
			if (output_number[0] == 1) {
				settings->setChannel0Voltage(static_cast<uint16_t>((target_volts.content.value)*1000), true);
			} else if (output_number[0] == 2) {
				settings->setChannel1Voltage(static_cast<uint16_t>((target_volts.content.value)*1000), true);
			}
			return SCPI_RES_OK;
		}
	} else {
		SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);
		return SCPI_RES_ERR;
	}
	return SCPI_RES_ERR;
}

scpi_result_t SCPIManager::DMM_ConfigureCurrent(scpi_t *context)
{
	UserContext *user_ctx = static_cast<UserContext *>(context->user_context);
	Settings *settings = user_ctx->settings;
	scpi_number_t target_amps;
	int32_t output_number[1];

	if (!SCPI_CommandNumbers(context, output_number, 1, 1)) {
		return SCPI_RES_ERR;
	}
	// required to accept MIN and MAX
	if (!SCPI_ParamNumber(context, scpi_special_numbers_def, &target_amps, TRUE)) {
		return SCPI_RES_ERR;
	}
	if (output_number[0] < 1 || output_number[0] > 2) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (target_amps.unit == SCPI_UNIT_NONE
			|| target_amps.unit == SCPI_UNIT_UNITLESS
			|| target_amps.unit == SCPI_UNIT_AMPER
	) {
		if (target_amps.special && target_amps.content.tag == SCPI_NUM_MIN) {
			if (output_number[0] == 1) {
				settings->setChannel0CurrentLimit(500, true);  // hardware allowed minimum
			} else if (output_number[0] == 2) {
				settings->setChannel1CurrentLimit(500, true);  // hardware allowed minimum
			}
			return SCPI_RES_OK;
		} else if (target_amps.special && target_amps.content.tag == SCPI_NUM_MAX) {
			if (output_number[0] == 1) {
				settings->setChannel0CurrentLimit(3000, true);  // hardware allowed maximum
			} else if (output_number[0] == 2) {
				settings->setChannel1CurrentLimit(3000, true);  // hardware allowed maximum
			}
			return SCPI_RES_OK;
		} else if (target_amps.content.value < 0.5 || target_amps.content.value > 3) {
			//SCPI_ErrorPush(context, SCPI_ERROR_DATA_OUT_OF_RANGE);  // full error list define
			SCPI_ErrorPush(context, SCPI_ERROR_DATA_TYPE_ERROR);  // full error list define
			return SCPI_RES_ERR;
		} else {
			Serial.println(output_number[0]);
			if (output_number[0] == 1) {
				settings->setChannel0CurrentLimit(static_cast<uint16_t>((target_amps.content.value)*1000), true);
			} else if (output_number[0] == 2) {
				settings->setChannel1CurrentLimit(static_cast<uint16_t>((target_amps.content.value)*1000), true);
			}
			return SCPI_RES_OK;
		}
	} else {
		SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);
		return SCPI_RES_ERR;
	}
	return SCPI_RES_ERR;
}

scpi_result_t SCPIManager::SCPI_NetworkMACQ(scpi_t *context)
{
	SCPI_ResultMnemonic(context, getSettings(context)->getMacAddress().c_str());
	return SCPI_RES_OK;
}

scpi_result_t SCPIManager::SCPI_NetworkDHCP (scpi_t *context)
{
	scpi_bool_t dhcp_enabled;
	// this parameter is mandatory, so error out if not present
	if (!SCPI_ParamBool(context, &dhcp_enabled, TRUE)) {
		return SCPI_RES_ERR;
	}

	getSettings(context)->setWifiIpv4DhcpEnabled(dhcp_enabled);
	return SCPI_RES_OK;
}

scpi_result_t SCPIManager::SCPI_NetworkDHCPQ (scpi_t *context)
{
	SCPI_ResultBool(context, getSettings(context)->isWifiIpv4DhcpEnabled());
	return SCPI_RES_OK;
}

scpi_result_t SCPIManager::SCPI_NetworkAddress (scpi_t *context)
{
	return SCPIManager::saveIpv4Address(context, &Settings::setWifiIpv4StaticIp);
}

scpi_result_t SCPIManager::SCPI_NetworkAddressQ (scpi_t *context)
{
	SCPI_ResultMnemonic(context, getSettings(context)->getWifiIpv4StaticIp(true).toString().c_str());
	return SCPI_RES_OK;

	//SCPI_ResultMnemonic(context, WiFi.localIP().toString().c_str());
	//return SCPI_RES_OK;
}

scpi_result_t SCPIManager::SCPI_NetworkGate (scpi_t *context)
{
	return SCPIManager::saveIpv4Address(context, &Settings::setWifiIpv4GatewayAddress);
}

scpi_result_t SCPIManager::SCPI_NetworkGateQ (scpi_t *context)
{
	SCPI_ResultMnemonic(context, getSettings(context)->getWifiIpv4GatewayAddress(true).toString().c_str());
	return SCPI_RES_OK;

	//SCPI_ResultMnemonic(context, WiFi.gatewayIP().toString().c_str());
	//return SCPI_RES_OK;
}

scpi_result_t SCPIManager::SCPI_NetworkSubnet (scpi_t *context)
{
	return SCPIManager::saveIpv4Address(context, &Settings::setWifiIpv4SubnetMask);
}

scpi_result_t SCPIManager::SCPI_NetworkSubnetQ (scpi_t *context)
{
	SCPI_ResultMnemonic(context, getSettings(context)->getWifiIpv4SubnetMask(true).toString().c_str());
	return SCPI_RES_OK;

	//SCPI_ResultMnemonic(context, WiFi.subnetMask().toString().c_str());
	//return SCPI_RES_OK;
}

scpi_result_t SCPIManager::SCPI_NetworkPort (scpi_t *context)
{
	return SCPIManager::saveNetworkPort(context, &Settings::setWifiIpv4SCPIServerPort);
}

scpi_result_t SCPIManager::SCPI_NetworkPortQ (scpi_t *context)
{
	SCPI_ResultUInt16(context, getSettings(context)->getWifiIpv4SCPIServerPort());
	return SCPI_RES_OK;
}

scpi_result_t SCPIManager::SCPI_NetworkHostnameQ (scpi_t *context)
{
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

scpi_result_t SCPIManager::SCPI_SocketPort (scpi_t *context)
{
	return SCPIManager::saveNetworkPort(context, &Settings::setWifiIpv4UdpLoggingServerPort);
}

scpi_result_t SCPIManager::SCPI_SocketPortQ (scpi_t *context)
{
	SCPI_ResultUInt16(context, getSettings(context)->getWifiIpv4UdpLoggingServerPort());
	return SCPI_RES_OK;
}

scpi_result_t SCPIManager::SCPI_SocketIPAddress(scpi_t *context)
{
	return SCPIManager::saveIpv4Address(context, &Settings::setWifiIpv4UdpLoggingServerIpAddress);
}

scpi_result_t SCPIManager::SCPI_SocketIPAddressQ(scpi_t *context)
{
	SCPI_ResultMnemonic(context, getSettings(context)->getWifiIpv4UdpLoggingServerIpAddress().toString().c_str());
	return SCPI_RES_OK;
}

Settings* SCPIManager::getSettings(scpi_t *context)
{
	return static_cast<UserContext *>(context->user_context)->settings;
}

scpi_result_t SCPIManager::saveIpv4Address(scpi_t *context, void (Settings::*func)(IPAddress, bool, bool))
{
	const char* value;
	uint len;
	IPAddress ipaddr_obj;
	char ipaddr[15];
	memset(ipaddr, 0x00, sizeof(ipaddr));

	if (!SCPI_ParamCharacters(context, &value, &len, TRUE)) {
		return SCPI_RES_ERR;
	}
	strncpy(ipaddr, value, len);
	if (ipaddr_obj.fromString(ipaddr)) {
		(getSettings(context)->* func)(ipaddr_obj, true, false);
		return SCPI_RES_OK;
	} else {
		//SCPI_ErrorPush(context, SCPI_ERROR_DATA_OUT_OF_RANGE);  // full error list
		SCPI_ErrorPush(context, SCPI_ERROR_DATA_TYPE_ERROR);  // full error list
		return SCPI_RES_ERR;
	}
	return SCPI_RES_ERR;
}

scpi_result_t SCPIManager::saveNetworkPort(scpi_t *context, void (Settings::*func)(uint16_t, bool, bool))
{
	scpi_number_t port;

	if (!SCPI_ParamNumber(context, scpi_special_numbers_def, &port, TRUE)) {
		return SCPI_RES_ERR;
	}

	if (0 <= port.content.value && port.content.value < 10000) {
		(getSettings(context)->* func)(port.content.value, true, false);
		return SCPI_RES_OK;
	} else {
		//SCPI_ErrorPush(context, SCPI_ERROR_DATA_OUT_OF_RANGE);  // full error list
		SCPI_ErrorPush(context, SCPI_ERROR_DATA_TYPE_ERROR);
		return SCPI_RES_ERR;
	}
}
