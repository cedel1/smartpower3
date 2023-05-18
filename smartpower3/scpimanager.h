#ifndef SMARTPOWER3_SCPIMANAGER_H_
#define SMARTPOWER3_SCPIMANAGER_H_


#include <SCPI_Parser.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <WiFi.h>
#include "settings.h"
#include "screenmanager.h"
#include <SCPI_Parser.h>
//#include "scpi-def.h"
#include <meas_channel.h>


#define SCPI_INPUT_BUFFER_LENGTH 256
#define SCPI_ERROR_QUEUE_SIZE 17
#define SCPI_IDN1 "Hardkernel Co Ltd"
#define SCPI_IDN2 "SmartPower3"
//#define SCPI_IDN3 "test"
//#define SCPI_IDN4 "01-02"

//extern const scpi_command_t scpi_commands[20];
//extern scpi_interface_t scpi_interface;
//extern char scpi_input_buffer[];
//extern scpi_error_t scpi_error_queue_data[];
//extern scpi_t scpi_context;

/*size_t SCPI_Write(scpi_t * context, const char * data, size_t len);
int SCPI_Error(scpi_t * context, int_fast16_t err);
scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val);
scpi_result_t SCPI_Reset(scpi_t * context);
scpi_result_t SCPI_Flush(scpi_t * context);*/


//scpi_result_t SCPI_SystemCommTcpipControlQ(scpi_t * context);


class UserContext
{
public:
	UserContext(ScreenManager *screen_manager, MeasChannels *measuring_channels);
	ScreenManager *screen_manager;
	Settings *settings;
	MeasChannels *measuring_channels;
	//Stream &stream_interface;
};


class SCPIManager
{
public:
	//SCPIManager(Settings *settings);
	SCPIManager(ScreenManager *screen_manager, MeasChannels *measuring_channels);
	~SCPIManager();
	void init(void);
	void processInput(Stream &communication_interface);
	//ScreenManager getScreenManager(void);

	static size_t SCPI_Write(scpi_t * context, const char * data, size_t len);
	static int SCPI_Error(scpi_t * context, int_fast16_t err);
	static scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val);
	static scpi_result_t SCPI_Reset(scpi_t * context);
	static scpi_result_t SCPI_Flush(scpi_t * context);
	static scpi_result_t SCPI_SystemCommTcpipControlQ(scpi_t * context);
	static scpi_result_t My_CoreTstQ(scpi_t * context);
	static scpi_result_t INST_SelectByNumber(scpi_t * context);
	static scpi_result_t INST_SelectByIdentifier(scpi_t * context);
	static scpi_result_t DeviceCapability(scpi_t * context);
	static scpi_result_t Reset(scpi_t * context);
	static scpi_result_t Output_TurnOnOff(scpi_t *context);
	static scpi_result_t Output_TurnOnOffQ(scpi_t *context);
	static scpi_result_t DMM_MeasureVoltageDcQ(scpi_t *context);
	static scpi_result_t DMM_ConfigureVoltage(scpi_t *context);

	char* getBuildDate(void);
	const char* getMacAddress(void);
protected:
private:
	UserContext *user_context;
	char build_date[34];
	String mac_address;
	char incoming_byte = 0; // for incoming serial data
	char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];
	scpi_error_t scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];
	scpi_t scpi_context;
	scpi_interface_t scpi_interface = {
		/*.error = */this->SCPI_Error,
		/*.write = */this->SCPI_Write,
		/*.control = */this->SCPI_Control,
		/*.flush = */this->SCPI_Flush,
		/*.reset = */this->SCPI_Reset,
	};
	const scpi_command_t scpi_commands[30] = {
		/* IEEE Mandated Commands (SCPI std V1999.0 4.1.1) */
		{ "*CLS", SCPI_CoreCls, 0 },
		{ "*ESE", SCPI_CoreEse, 0 },
		{ "*ESE?", SCPI_CoreEseQ, 0 },
		{ "*ESR?", SCPI_CoreEsrQ, 0 },
		{ "*IDN?", SCPI_CoreIdnQ, 0 },
		{ "*OPC", SCPI_CoreOpc, 0 },
		{ "*OPC?", SCPI_CoreOpcQ, 0 },
//TODO: Check if reset is complete or needs rework
		//{ "*RST", SCPI_CoreRst, 0 },
		{ "*RST", Reset, 0 },
		{ "*SRE", SCPI_CoreSre, 0 },
		{ "*SRE?", SCPI_CoreSreQ, 0 },
		{ "*STB?", SCPI_CoreStbQ, 0 },
		{ "*TST?", My_CoreTstQ, 0 },
		{ "*WAI", SCPI_CoreWai, 0 },

		/* Required SCPI commands (SCPI std V1999.0 4.2.1) */
		{ "SYSTem:ERRor[:NEXT]?", SCPI_SystemErrorNextQ, 0 },
		{ "SYSTem:ERRor:COUNt?", SCPI_SystemErrorCountQ, 0 },
		{ "SYSTem:VERSion?", SCPI_SystemVersionQ, 0 },
		/* Non required SYSTem commands */
		{ "SYSTem:CAPability?", DeviceCapability, 0 },

		//{"STATus:OPERation?", scpi_stub_callback, 0},
		//{"STATus:OPERation:EVENt?", scpi_stub_callback, 0},
		//{"STATus:OPERation:CONDition?", scpi_stub_callback, 0},
		//{"STATus:OPERation:ENABle", scpi_stub_callback, 0},
		//{"STATus:OPERation:ENABle?", scpi_stub_callback, 0},

		{ "STATus:QUEStionable[:EVENt]?", SCPI_StatusQuestionableEventQ, 0 },
		//{"STATus:QUEStionable:CONDition?", scpi_stub_callback, 0},
		{ "STATus:QUEStionable:ENABle", SCPI_StatusQuestionableEnable, 0 },
		{ "STATus:QUEStionable:ENABle?", SCPI_StatusQuestionableEnableQ, 0 },

		{ "STATus:PRESet", SCPI_StatusPreset, 0 },

		/* DMM */
		{ "MEASure[:SCALar]:VOLTage:DC?", DMM_MeasureVoltageDcQ, 0 },

		//{"[SOURce]:CURRent", DMM_ConfigureCurrent, 0 },
		{"[SOURce]:VOLTage", DMM_ConfigureVoltage, 0 },

/*		{ "CONFigure:VOLTage:DC", DMM_ConfigureVoltageDc, 0 },
		{ "MEASure:VOLTage:DC:RATio?", SCPI_StubQ, 0 },
		{ "MEASure:VOLTage:AC?", DMM_MeasureVoltageAcQ, 0 },
		{ "MEASure:CURRent:DC?", SCPI_StubQ, 0 },
		{ "MEASure:CURRent:AC?", SCPI_StubQ, 0 },
		{ "MEASure:RESistance?", SCPI_StubQ, 0 },
		{ "MEASure:FRESistance?", SCPI_StubQ, 0 },
		{ "MEASure:FREQuency?", SCPI_StubQ, 0 },
		{ "MEASure:PERiod?", SCPI_StubQ, 0 },*/

		//{ "INSTrument:NSELect", INST_SelectByNumber, 0 },
		//{ "INSTrument:SELect", INST_SelectByIdentifier, 0 },

		{ "OUTPut[:STATe]", Output_TurnOnOff, 0 },
		{ "OUTPut[:STATe]?", Output_TurnOnOffQ, 0 },

		/*{ "SYSTem:COMMunication:TCPIP:CONTROL?", SCPI_SystemCommTcpipControlQ, 0 },*/

		/*{ "TEST:BOOL", TEST_Bool, 0 },
		{ "TEST:CHOice?", TEST_ChoiceQ, 0 },
		{ "TEST#:NUMbers#", TEST_Numbers, 0 },
		{ "TEST:TEXT", TEST_Text, 0 },
		{ "TEST:ARBitrary?", TEST_ArbQ, 0 },
		{ "TEST:CHANnellist", TEST_Chanlst, 0 },*/

		SCPI_CMD_LIST_END
	};
};


#endif /* SMARTPOWER3_SCPIMANAGER_H_ */
