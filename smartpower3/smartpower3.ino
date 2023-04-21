#include <Wire.h>
#include "smartpower3.h"
//#include "events.h"

uint32_t ctime1 = 0;

volatile int interruptFlag;

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;


#include "esp_event.h"
#include "esp_event_base.h"
//#include "screen.h"
//esp_event_loop_handle_t loop_with_task;

ESP_EVENT_DEFINE_BASE(SETTINGS_EVENTS);

//enum {
    //SETTINGS_VOLTAGE0_CHANGED_EVENT,
//};



void IRAM_ATTR onTimer()
{
	portENTER_CRITICAL_ISR(&timerMux);
	interruptFlag++;
	portEXIT_CRITICAL_ISR(&timerMux);
}


static void settings_voltage0_changed_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
	Serial.printf("voltage set (normal): %d\n\r", settings.getChannel0Voltage());
	Serial.printf("voltage set (forced): %d\n\r", settings.getChannel0Voltage(true));

	// following does roughly what is done when settings value through screen UI
	// with the exception that absolute value is set (not incremental)
	//screen.getChannel(0)->editVolt(settings.getChannel0Voltage(true), true);
	screen.getChannel(0)->setVolt(settings.getChannel0Voltage(true), 2);
	//Serial.printf("voltage set after change: %d\n\r", settings.getChannel0Voltage(true));
}



void setup(void) {
	Serial.begin(115200);

	delay(100);  // needed for the following not to block
	delay(8000);

	settings.init();
	Serial.printf("Settings address: %p\n\r", &settings);
//	used_settings = settings;

	esp_event_loop_args_t loop_with_task_args = {
			.queue_size = 5,
			.task_name = "loop_task", // task will be created
			.task_priority = uxTaskPriorityGet(NULL),
			.task_stack_size = 3072,
			.task_core_id = tskNO_AFFINITY
	};
	esp_event_loop_handle_t& loop_with_task = settings.getEventLoopHandleAddress();
	Serial.printf("loop_with_task: %p\n\r");
	esp_event_loop_create(&loop_with_task_args, &loop_with_task);

	I2CA.begin(15, 4, (uint32_t)10000);
	I2CB.begin(21, 22, (uint32_t)800000);
	PAC.begin(&I2CB);
	screen.begin(&settings, &I2CA);

	//settings.setChannel(screen.getChannel[0], 0);
	//settings.setChannel(screen.getChannel[1], 1);
	initEncoder(&dial);  // this also starts a task, without specified core

	xTaskCreatePinnedToCore(screenTask, "Draw Screen", 6000, NULL, 1, NULL, 1);  // delay 10
	xTaskCreatePinnedToCore(wifiTask, "WiFi Task", 5000, NULL, 1, NULL, 1);  // delay 50
	xTaskCreatePinnedToCore(logTask, "Log Task", 8000, NULL, 1, NULL, 1);  // delay 10, 250 or 1 depending on logging interval and interrupt count
	xTaskCreate(inputTask, "Input Task", 8000, NULL, 1, NULL);  // delay 10, also counts for screen
	xTaskCreate(btnTask, "Button Task", 4000, NULL, 1, NULL);  // delay 10
	//xTaskCreate(voltageChangeTask, "Voltage Task", 4000, (void *)&settings, 1, NULL);  // delay 10
	//xTaskCreatePinnedToCore(voltageChangeTask, "Voltage Task", 4000, NULL, 1, NULL, 1);  // delay 10

	pinMode(25, INPUT_PULLUP);
	pinMode(26, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(25), isr_stpd01_ch0, FALLING);
	attachInterrupt(digitalPinToInterrupt(26), isr_stpd01_ch1, FALLING);

	timer = timerBegin(0, 80, true);
	timerAttachInterrupt(timer, &onTimer, true);
	timerAlarmWrite(timer, 1000000, true);
	timerAlarmEnable(timer);

	//esp_event_loop_create_default();
	esp_event_handler_instance_register_with((esp_event_loop_handle_t)loop_with_task, SETTINGS_EVENTS, SETTINGS_VOLTAGE0_CHANGED_EVENT, settings_voltage0_changed_handler, NULL, NULL);
}


void voltageChangeTask(void *parameter)
{
	delay(20000);
	uint16_t voltage0;
	//Settings settings = *((Settings *)parameter);

	voltage0 = settings.getChannel0Voltage();
	for (;;) {
		while (voltage0 < 20000) {
			voltage0 = settings.getChannel0Voltage();
			Serial.printf(">>> Changing Voltage of channel 0 to %d<<<\n\r", voltage0+100);
			voltage0 += 100;
			settings.setChannel0Voltage(voltage0, true);
			delay(10000);
		}
		//vTaskDelay(100);
	}
}


void isr_stpd01_ch0()
{
	screen.setIntFlag(0);
}

void isr_stpd01_ch1()
{
	screen.setIntFlag(1);
}

void initPAC1933(void)
{
	PAC.UpdateProductID();
	PAC.UpdateManufacturerID();
	PAC.UpdateRevisionID();
}

void btnTask(void *parameter)
{
	for (;;) {
		for (int i = 0; i < 4; i++)
			button[i].isr_pol();
		vTaskDelay(10);
	}
}

void logTask(void *parameter)
{
	char buffer_input[SIZE_LOG_BUFFER0];
	char buffer_ch0[SIZE_LOG_BUFFER1];
	char buffer_ch1[SIZE_LOG_BUFFER2];
	char buffer_checksum[SIZE_CHECKSUM_BUFFER];
	uint32_t log_interval = 0;
	uint8_t checksum8 = 0;
	uint8_t checksum8_xor = 0;

	for (;;) {
		if (interruptFlag > 0) {
			portENTER_CRITICAL(&timerMux);
			interruptFlag--;
			portEXIT_CRITICAL(&timerMux);
			if (log_interval != screen.getEnabledLogInterval()) {
				log_interval = screen.getEnabledLogInterval();
				if (log_interval == 0)
					timerAlarmWrite(timer, 250000, true);
				else
					timerAlarmWrite(timer, 1000*log_interval, true);
			}
			if ((log_interval > 0) && !screen.wifiManager->isCommandMode()) {
				sprintf(buffer_input, "%010lu,%05d,%04d,%05d,%1d,", millis(), mCh0.V(),mCh0.A(log_interval), mCh0.W(log_interval), low_input);
				sprintf(buffer_ch0, "%05d,%04d,%05d,%1d,%02x,", mCh1.V(), mCh1.A(log_interval), mCh1.W(log_interval), onoff[0], screen.getIntStat(0));
				sprintf(buffer_ch1, "%05d,%04d,%05d,%1d,%02x,", mCh2.V(), mCh2.A(log_interval), mCh2.W(log_interval), onoff[1], screen.getIntStat(1));

				checksum8 = 0;
				checksum8_xor = 0;
				for (int i = 0; i < SIZE_LOG_BUFFER0-1; i++) {
					checksum8 += buffer_input[i];
					checksum8_xor ^= buffer_input[i];
				}
				for (int i = 0; i < SIZE_LOG_BUFFER1-1; i++) {
					checksum8 += buffer_ch0[i];
					checksum8_xor ^= buffer_ch0[i];
				}
				for (int i = 0; i < SIZE_LOG_BUFFER2-1; i++) {
					checksum8 += buffer_ch1[i];
					checksum8_xor ^= buffer_ch1[i];
				}
				sprintf(buffer_checksum, "%02x,%02x\r\n", (byte)((~checksum8)+1), checksum8_xor);

				Serial.printf(buffer_input);
				Serial.printf(buffer_ch0);
				Serial.printf(buffer_ch1);
				Serial.printf(buffer_checksum);
				screen.runWiFiLogging(buffer_input, buffer_ch0, buffer_ch1, buffer_checksum);
			} else {
				vTaskDelay(10);
			}
		} else if (log_interval == 0) {
			vTaskDelay(250);
		} else if (log_interval > 5) {
			vTaskDelay(1);
		}
	}
}

void screenTask(void *parameter)
{
	wl_status_t old_state = WL_IDLE_STATUS;
	for (;;) {
		if (old_state != WiFi.status()) {
			old_state = WiFi.status();
			screen.updateWiFiInfo();
		}

		screen.run();
		vTaskDelay(10);
	}
}

void inputTask(void *parameter)
{
	uint8_t pressed;
	unsigned long cur_time;
	for (;;) {
		cur_time = millis();
		for (int i = 0; i < 4; i++) {
			pressed = button[i].checkPressed();
			if (pressed == 1)
				screen.getBtnPress(i, cur_time);
			else if (pressed == 2)
				screen.getBtnPress(i, cur_time, true);
		}
		if (dial.cnt != 0) {
			screen.countDial(dial.cnt, dial.direct, dial.step, cur_time);
			dial.cnt = 0;
		}
		screen.setTime(cur_time);
		vTaskDelay(10);
	}
}

void wifiTask(void *parameter)
{
	for (;;) {
		if (screen.wifiManager->canConnect() && screen.isWiFiEnabled()) {
			screen.wifiManager->apConnectFromSettings();
		} else if (!screen.isWiFiEnabled()) {
			screen.wifiManager->apDisconnectAndTurnWiFiOff();
		}
		screen.setWiFiIconState();

		if (Serial.available()) {
			if (screen.wifiManager->isCommandMode())
				screen.wifiManager->WiFiMenuMain(Serial.read());
			else {
				if (Serial.read() == SERIAL_CTRL_C) {
					screen.wifiManager->setCommandMode();
					screen.wifiManager->viewMainMenu();
				} else {
					Serial.printf(">>> Unknown command <<<\n\r");
				}
			}
		}
		vTaskDelay(50);
	}
}

void loop() {
	onoff = screen.getOnOff();

	mChs.sample();

	if ((millis() - ctime1) > 300) {
		ctime1 = millis();
		if (mCh0.V() < 6000) {
			screen.debug();
			for (int i = 0; i < 3; i++) {
				mChs.sample();
				if (mCh0.V() > 6000) {
					break;
				}
				low_input = true;
			}
		} else {
			low_input = false;
		}
		screen.pushInputPower(mCh0.V(), mCh0.A(300), mCh0.W(300));
		screen.pushPower(mCh1.V(), mCh1.A(300), mCh1.W(300), 0);
		screen.pushPower(mCh2.V(), mCh2.A(300), mCh2.W(300), 1);

		if (!screen.getShutdown()) {
			if ((millis()/1000)%2)
				screen.writeSysLED(50);
			else
				screen.writeSysLED(0);
			screen.writePowerLED(50);
		}
	}

	if (screen.getShutdown()) {
		screen.dimmingLED(1);
	}
}
