#include "monitor.h"

#define CAN_LED 9

static uint8_t can_led_status = 0;

Can_monitor::Can_monitor(MCP_CAN& c) :
		m_can(c), m_global_timer(millis()) {

	/*
	 * Reset frame storage
	 */
	memset(m_uch_general, 0, sizeof(m_uch_general));
	memset(m_cluster_backup, 0, sizeof(m_cluster_backup));
	memset(m_brake_data, 0, sizeof(m_brake_data));
	memset(m_upc_data, 0, sizeof(m_upc_data));
	memset(m_engine_general, 0, sizeof(m_engine_general));
	memset(m_uch_system, 0, sizeof(m_uch_system));

	m_data_available = 0;

	pinMode(CAN_LED, OUTPUT);
	digitalWrite(CAN_LED, 1);
}

Can_monitor::~Can_monitor() {

}

void
Can_monitor::handle_ecm() {
	uint8_t fuel_data = m_engine_general[1];
	uint8_t fuel_diff = fuel_data - m_last_fuel_data;
	if (fuel_diff == 0) return;
	unsigned long current_time = millis();
	unsigned long diff_time = current_time - m_engine_last_timestamp;

	m_fuel_acc += fuel_diff;

	if (diff_time > 1000) {
		m_mm3perhour = (m_fuel_acc * 288000) / (diff_time);
		m_fuel_acc = 0;
		m_fuel_stat_available = true;
		m_engine_last_timestamp = current_time;
	}

	m_last_fuel_data = fuel_data;
}

/*
 * 5.6 30
 *     100
 */

String
Can_monitor::get_instant_consumption()
{
	char buf[20];
	float speed = get_speed_raw();

	// convert mm3 to liters (1L = 1dm3) 1dm3 = 1e6mm3
	float dm3perhour = m_mm3perhour;
	dm3perhour *= 1e-3f;
	if (speed > 3000){
		return dtostrf((dm3perhour * 10000.f) / speed, 4, 1, buf);
	} else {
		return dtostrf(dm3perhour, 4, 1, buf);
	}
}

String
Can_monitor::get_instant_consumption_unit()
{
	float speed = get_speed_raw();
	if (speed > 3000){
		return "100";
	} else {
		return "h  ";
	}
}

bool
Can_monitor::monitor() {
	// Make a hard copy of the vehicle real time parameters
	// that can be used for later use
	uint8_t recv_frame[8];

	unsigned long can_id;
	uint8_t len;
	while (m_can.readMsgBuf(&can_id, &len, recv_frame) == CAN_OK) {
		switch (can_id) {
		case CANID_UCH_GENERAL:
			m_data_available |= 0b00000001;
			memcpy(m_uch_general, recv_frame, len);
			break;
		case CANID_UCH_SYSTEM:
			m_data_available |= 0b00000010;
			memcpy(m_uch_system, recv_frame, len);
			break;
		case CANID_CLUSTER_BACKUP:
			m_data_available |= 0b00000100;
			memcpy(m_cluster_backup, recv_frame, len);
			break;
		case CANID_BRAKE_DATA:
			m_data_available |= 0b00001000;
			memcpy(m_brake_data, recv_frame, len);
			break;
		case CANID_UPC_GENERAL:
			m_data_available |= 0b00010000;
			memcpy(m_upc_data, recv_frame, len);
			break;
		case CANID_ECM_GENERAL:
			can_led_status = !can_led_status;
			digitalWrite(CAN_LED, can_led_status);
			m_data_available |= 0b00100000;
			memcpy(m_engine_general, recv_frame, 8);
			handle_ecm();
			break;
		default:
			break;
		}
		++m_frame_count;
	}
	return true;
}

