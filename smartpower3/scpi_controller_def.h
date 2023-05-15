

SCPI_Controller::SCPI_Controller(Settings *settings)
{
	//my_instrument = new SCPI_Parser();
	this->settings = settings;
}

void SCPI_Controller::Identify(SCPI_C commands, SCPI_P parameters, Stream& interface)
{
	interface.printf(
			"Hardkernel Co Ltd, SmartPower3, MAC Address: %s, Build Date %s %s%s",
			WiFi.macAddress().c_str(),
			String(__DATE__).c_str(),
			String(__TIME__).c_str(),
			LINE_END);
}

void SCPI_Controller::DoNothing(SCPI_C commands, SCPI_P parameters, Stream& interface)
{
	interface.println(F("Not implemented"));
}

void DoSpe(SCPI_C commands, Stream& interface)
{
}

void SCPI_Controller::init(void)
{
	/*
	To fix hash crashes, the hashing magic numbers can be changed before
	registering commands.
	Use prime numbers up to the SCPI_HASH_TYPE size.
	*/
	my_instrument.hash_magic_number = 37; //Default value = 37
	my_instrument.hash_magic_offset = 7;  //Default value = 7

	/*
	Timeout time can be changed even during program execution
	See Error_Handling example for further details.
	*/
	my_instrument.timeout = 1000; //value in miliseconds. Default value = 10

	my_instrument.SetCommandTreeBase(F("STATus:OPERation"));
		my_instrument.RegisterCommand(F(":CONDition?"), &DoNothing);
		my_instrument.RegisterCommand(F(":ENABle"), &DoNothing);
		my_instrument.RegisterCommand(F(":EVENt?"), &DoNothing);
	my_instrument.SetCommandTreeBase(F("STATus:QUEStionable"));
		my_instrument.RegisterCommand(F(":CONDition?"), &DoNothing);
		my_instrument.RegisterCommand(F(":ENABle"), &DoNothing);
		my_instrument.RegisterCommand(F(":EVENt?"), &DoNothing);
	my_instrument.SetCommandTreeBase(F("STATus"));
		my_instrument.RegisterCommand(F(":OPERation?"), &DoNothing);
		my_instrument.RegisterCommand(F(":QUEStionable?"), &DoNothing);
		my_instrument.RegisterCommand(F(":PRESet"), &DoNothing);
	my_instrument.SetCommandTreeBase(F("SYSTem"));
		my_instrument.RegisterCommand(F(":ERRor?"), &DoNothing);
		my_instrument.RegisterCommand(F(":ERRor:NEXT?"), &DoNothing);
		my_instrument.RegisterCommand(F(":VERSion?"), &DoNothing);
	my_instrument.SetCommandTreeBase(F(""));
	// Required common commands

	// Clear Status Command
	my_instrument.RegisterCommand(F("*CLS"), &DoNothing);
	// Standard Event Status Enable Command
	my_instrument.RegisterCommand(F("*ESE"), &DoNothing);
	// Standard Event Status Enable Query
	my_instrument.RegisterCommand(F("*ESE?"), &DoNothing);
	// Standard Event Status Register Query (0-255)
	my_instrument.RegisterCommand(F("*ESR"), &DoNothing);
	// Identification Query (Company, model, serial number and revision)
	my_instrument.RegisterCommand(F("*IDN?"), &SCPI_Controller::Identify);
	// Operation Complete Command
	my_instrument.RegisterCommand(F("*OPC"), &DoNothing);
	// Operation Complete Query
	my_instrument.RegisterCommand(F("*OPC?"), &DoNothing);
	// Reset Command
	my_instrument.RegisterCommand(F("*RST"), &DoNothing);
	// Service Request Enable Command
	my_instrument.RegisterCommand(F("*SRE"), &DoNothing);
	// Service Request Enable Query (0-255)
	my_instrument.RegisterCommand(F("*SRE?"), &DoNothing);
	// Status Byte Query Z (0-255)
	my_instrument.RegisterCommand(F("*STB"), &DoNothing);
	// Self-Test Query
	my_instrument.RegisterCommand(F("*TST?"), &DoNothing);
	// Wait-to-Continue Command
	my_instrument.RegisterCommand(F("*WAI"), &DoNothing);

	Serial.flush();
	Serial.end();
	Serial.begin(settings->getSerialBaudRate(true));
	//while (!Serial) {;}

	/*
	void SCPI_Parser::PrintDebugInfo(Stream &interface)
	Will print any set up error to a Stream interface.
	Use it only to test your setup  (not needed for normal execution).
	*/
	//my_instrument.PrintDebugInfo(Serial);
}

void SCPI_Controller::processInput(void)
{
	/*
	void SCPI_Parser::ProcessInput(Stream &interface, const char *term_chars)
	will process any message from a stream inteface.
	The used interface will be passed to the Registered command handlers.
	*/
	my_instrument.ProcessInput(Serial, LINE_END);

	/*
	void SCPI_Parser::Execute(char *message, Stream &interface)
	can also be used to process a message.
	Use this if the message source is not an Stream.
	A stream like object is still needed for passing it to the command handlers.
	i.e.
		my_instrument.Execute(GetEthernetMsg(), Serial)
	*/
}
