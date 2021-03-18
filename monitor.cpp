#include "monitor.h"

#define CAN_LED 9

extern unsigned long g_can_last_int;

static uint8_t can_led_status = 0;

Can_monitor::Can_monitor(MCP_CAN& c) :
		m_can(c), m_global_timer(millis()) {
	/*
	 * MASK 0 -> FILTER 0, 1
	 * MASK 1 -> FILTER 2, 3, 4, 5
	 */
	m_can.init_Mask(0, false, 0x7FF);
	m_can.init_Mask(0, false, 0x7FF);
	m_can.init_Filt(0, false, CANID_UCH_GENERAL);
	m_can.init_Filt(1, false, CANID_CLUSTER_BACKUP);
	m_can.init_Filt(2, false, CANID_ECM_GENERAL);
	m_can.init_Filt(3, false, CANID_BRAKE_DATA);
	m_can.init_Filt(4, false, CANID_UPC_GENERAL);
	m_can.init_Filt(5, false, CANID_UCH_SYSTEM);

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
Can_monitor::handle_ecm(uint8_t data[]) {
	unsigned long current_time = millis();
	unsigned long diff_time = current_time - m_engine_last_timestamp;
	memcpy(m_engine_general, data, 8);
	uint8_t fuel_data = m_engine_general[1];
	uint8_t fuel_diff = fuel_data - m_last_fuel_data;

	m_fuel_acc += fuel_diff;
	m_fuel_accum_time += diff_time;

	if (m_fuel_accum_time > 1500) {
		m_mm3perhour = (m_fuel_acc * 288000000) / (m_fuel_accum_time);
		m_fuel_accum_time = 0;
		m_fuel_acc = 0;
		m_fuel_stat_available = true;
	}

	m_engine_last_timestamp = current_time;
	m_last_fuel_data = fuel_data;
}

String
Can_monitor::get_instant_consumption()
{
	String result;
	if (m_mm3perhour > 0){
		float speed = get_speed_raw();

		// convert mm3 to liters (1L = 1dm3) 1dm3 = 1e6mm3
		float dm3perhour = m_mm3perhour;
		dm3perhour *= 1e-6f;
		if (speed > 3000){
			result.concat((10000.f/speed) * dm3perhour);
			result.concat("L/100");
		} else {
			result.concat(dm3perhour);
			result.concat("L/h");
		}
	} else {
		result.concat("---");
	}
	return result;
}

bool
Can_monitor::monitor() {
	// Make a hard copy of the vehicle real time parameters
	// that can be used for later use
	uint8_t recv_frame[8];

	unsigned long can_id;
	uint8_t len;
	if (m_can.readMsgBuf(&can_id, &len, recv_frame) == CAN_OK) {
		can_led_status = !can_led_status;
		digitalWrite(CAN_LED, can_led_status);
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
			m_data_available |= 0b00100000;
			handle_ecm(recv_frame);
			break;
		default:
			break;
		}
		++m_frame_count;
		return true;
	}
	return false;

}

