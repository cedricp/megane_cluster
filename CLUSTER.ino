#include "Arduino.h"
#include "mcp_can.h"
#include "monitor.h"
#include "SSD1351.h"
#include "board.h"
#include "lowpower.h"
#include "io.h"

/*
                  +-___-+
      (RES_)PC6  1|     |28  PC5 (AI 5) (D 19)
      (D 0) PD0  2|     |27  PC4 (AI 4) (D 18)
      (D 1) PD1  3|     |26  PC3 (AI 3) (D 17)
  INT0(D 2) PD2  4|     |25  PC2 (AI 2) (D 16)
  INT1(D 3) PD3  5|  A  |24  PC1 (AI 1) (D 15)
      (D 4) PD4  6|  T  |23  PC0 (AI 0) (D 14)
            VCC  7|  3  |22  GND
            GND  8|  2  |21  AREF
            PB6  9|  8  |20  AVCC
            PB7 10|     |19  PB5 (D 13)    | SCK  -+
 PWM+ (D 5) PD5 11|     |18  PB4 (D 12)    | MISO  |--> SPI
 PWM+ (D 6) PD6 12|     |17  PB3 (D 11) PWM| MOSI -+
      (D 7) PD7 13|     |16  PB2 (D 10) PWM
      (D 8) PB0 14|     |15  PB1 (D 9)  PWM
                  +-----+
*/

#define SSD1351_CS_PIN 		8
#define SSD1351_DC_PIN 		7
#define SSD1351_RST_PIN 	6

#define MCP_CAN_CS 			10
#define MCP_CAN_INT 		2 // INT0

#define CAN_LED 9
#define TPMS_POWER 5

#define MCP_CAN_BAUDRATE  	CAN_500KBPS
#define MCP_OSC_SPEED 		MCP_8MHZ

#define INTERNAL_SWITCH_PIN 4

MCP_CAN can(MCP_CAN_CS);
SSD1351 display(SSD1351_CS_PIN, SSD1351_DC_PIN, SSD1351_RST_PIN);
Can_monitor monitor(can);
Board board(display, monitor);

bool g_is_sleeping = false;
int g_can_status = CAN_FAIL;

DigitalPin button(INTERNAL_SWITCH_PIN, DigitalPin::MODE_INPUT_PULLUP);

void init_can_filter_normal()
{
	/*
	 * MASK 0 -> FILTER 0, 1
	 * MASK 1 -> FILTER 2, 3, 4, 5
	 */
	can.init_Mask(0, 0x7FF0000);
	can.init_Mask(1, 0x7FF0000);

	can.init_Filt(0, (unsigned long)(CANID_UCH_GENERAL) << 16);
	can.init_Filt(1, (unsigned long)(CANID_BRAKE_DATA) << 16);
	can.init_Filt(2, (unsigned long)(CANID_CLUSTER_BACKUP) << 16);
	can.init_Filt(3, (unsigned long)(CANID_ECM_GENERAL) << 16);
	can.init_Filt(4, (unsigned long)(CANID_UPC_GENERAL) << 16);
	can.init_Filt(5, (unsigned long)(CANID_UCH_SYSTEM) << 16);
}

void init_can_filter_sleep()
{
	can.init_Mask(0, 0x7FF0000);
	can.init_Mask(1, 0x7FF0000);

	can.init_Filt(0, (unsigned long)(CANID_UCH_SYSTEM) << 16);
	can.init_Filt(1, (unsigned long)(CANID_UCH_SYSTEM) << 16);
	can.init_Filt(2, (unsigned long)(CANID_UCH_SYSTEM) << 16);
	can.init_Filt(3, (unsigned long)(CANID_UCH_SYSTEM) << 16);
	can.init_Filt(4, (unsigned long)(CANID_UCH_SYSTEM) << 16);
	can.init_Filt(5, (unsigned long)(CANID_UCH_SYSTEM) << 16);
}

void init_can()
{
	pinMode(MCP_CAN_INT, INPUT);
	g_can_status = can.begin(MCP_STDEXT, MCP_CAN_BAUDRATE, MCP_OSC_SPEED);
	if (g_can_status != CAN_OK){
		return;
	}

	init_can_filter_normal();

	can.setMode(MCP_NORMAL);
}

// void can_interrupt_handler()
// {
// 	g_can_last_int = millis();
// 	g_can_data_availabe = true;
// }

void init_screen()
{
	// display.begin -> send reset signal
	display.begin(0);
	display.enableDisplay(true);
	display.fillScreen(0);
	display.setCursor(0, 0);
	display.setRotation(2);
}

void setup()
{
	Serial.begin(19200, SERIAL_8N1);
	init_screen();
	display.setTextColor(SSD1351::WHITE);
	display.println("** Megane CC **");
	display.print("Free RAM:");
	display.println(monitor.freeRam());
	display.println("CAN BUS init...");

	pinMode(14, OUTPUT);
	pinMode(15, OUTPUT);
	pinMode(16, OUTPUT);
	pinMode(17, OUTPUT);
	pinMode(18, OUTPUT);
	pinMode(19, OUTPUT);

	digitalWrite(14, 0);
	digitalWrite(15, 0);
	digitalWrite(16, 0);
	digitalWrite(17, 0);
	digitalWrite(18, 0);
	digitalWrite(19, 0);
	
	pinMode(TPMS_POWER, OUTPUT);
	digitalWrite(TPMS_POWER, 1);

	init_can();

	if (g_can_status != CAN_OK){
		display.setTextColor(SSD1351::RED);
		display.println("CAN bus failed");
	} else {
		display.setTextColor(SSD1351::GREEN);
		display.println("CAN Bus OK !");
	}

	Serial.write("\x55\xaa\x06\x07\x00\xfe", 6);

	// Wait for 'TPMS get id' command to proceed
	for(int i = 0; i < 200; i++){
		get_serial();
		delay(10);
	}
	
	char buf[32];
	display.setTextColor(SSD1351::WHITE);
	for(int i=0; i < 4; ++i){
		tireInfo& nfo = board.getTireInfo(i);
		sprintf(buf, "Tire ID %d: %02x%02x%02x%02x", i, nfo.id[0], nfo.id[1], nfo.id[2], nfo.id[3]);
		display.println(buf);
	}

	delay(2000);
	board.init();
}

void get_serial()
{
	static bool led_status;
	static uint8_t buffer[10];
	static uint8_t bufpos = 0;
	static uint8_t buflen = 0;
	bool full_frame = false;
	while(Serial.available() > 0){
		uint8_t byte = Serial.read();
		if (bufpos == 0 && byte == 0x55){
			++bufpos;
			continue;
		}
		if (bufpos == 1 && byte == 0xaa){
			++bufpos;
			continue;
		} else if (bufpos == 1){
			bufpos = 0;
		}
		if (bufpos > 1){
			buffer[bufpos-2] = byte;
			if (bufpos == 2){
				if (byte > 9){
					bufpos = 0;
					continue;
				}
				buflen = byte;
			}
			++bufpos;
		}
		if (bufpos >= buflen){
			led_status = ! led_status;
			digitalWrite(CAN_LED, led_status);
			if (buflen == 8){
				uint8_t tirepos = buffer[1];
				if(tirepos == 0x10) tirepos = 2;
				else if (tirepos == 0x11) tirepos = 3;

				if (tirepos < 4){
					tireInfo& ti = board.getTireInfo(tirepos);
					ti.pressure = buffer[2];
					ti.temp = buffer[3] - 50;
					ti.leak = buffer[4] & 0b00001000;
					ti.nosignal = buffer[4] & 0b10000000;
					ti.lowbatt = buffer[4] & 0b00010000;
					ti.up2date = true;
				}
			} else if (buflen == 9){
				uint8_t tireid = buffer[1];
				if (tireid < 5 && tireid >0){
					board.getTireInfo(tireid-1).id[0] = buffer[2];
					board.getTireInfo(tireid-1).id[1] = buffer[3];
					board.getTireInfo(tireid-1).id[2] = buffer[4];
					board.getTireInfo(tireid-1).id[3] = buffer[5];
				}
			}
			bufpos = 0;
		}
	}
}

void loop()
{
	static bool check_hack = true;

	if (button.get() == false){
		digitalWrite(TPMS_POWER, 1);
		init_screen();
		board.init();
		display.enableDisplay(true);
		init_can();
		Serial.begin(19200, SERIAL_8N1);
		g_is_sleeping = false;
		check_hack = false;
	}

	if (g_can_status != CAN_OK){
		init_can();
	}

	if (g_is_sleeping){
		// Try to draw minimum current...
		LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);  

		bool must_wake = true;
		if (g_can_status == CAN_OK){
			monitor.monitor();
			// Keep led off (power saving...)
			digitalWrite(CAN_LED, 0);
		}
		must_wake = !monitor.must_sleep();

		if (must_wake){
			digitalWrite(TPMS_POWER, 1);
			init_screen();
			board.init();
			display.enableDisplay(true);
			init_can();
			Serial.begin(19200, SERIAL_8N1);
			g_is_sleeping = false;
		}
	} else {
		get_serial();
		board.update();
		
		if (check_hack){
			// Check sleep mode
			bool must_sleep = monitor.must_sleep();

			if (must_sleep){
				display.enableDisplay(false);
				// Get only sleep signal frame from CAN
				init_can_filter_sleep();
				digitalWrite(CAN_LED, 0);
				digitalWrite(TPMS_POWER, 0);
				Serial.end();
				// Put TX line to 0V 
				pinMode(1, OUTPUT);
				digitalWrite(1, 0);
				g_is_sleeping = true;
			}
		}
	}
}
