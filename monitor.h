#ifndef MONITOR_H
#define MONITOR_H

#include "mcp_can.h"

enum ecu_ids {
	CANID_UCH_SYSTEM 		= 0x35d,
	CANID_UCH_GENERAL 		= 0x60d,
	CANID_CLUSTER_BACKUP 	= 0x715,
	CANID_ECM_GENERAL 		= 0x551,
	CANID_BRAKE_DATA 		= 0x354,
	CANID_UPC_GENERAL 		= 0x625
};

enum cruise_control_ids {
	OFF,
	LIMITER_ACTIVE,
	LIMITER_INACTIVE,
	LIMITER_FAULT,
	CC_ACTIVE,
	CC_INACTIVE,
	CC_FAULT,
	NOT_PRESENT
};

class Can_monitor {
	MCP_CAN& m_can;
	uint8_t m_uch_general[8];
	uint8_t m_uch_system[8];
	uint8_t m_cluster_backup[8];
	uint8_t m_engine_general[8];
	uint8_t m_brake_data[8];
	uint8_t m_upc_data[8];
	uint8_t m_data_available;

	unsigned long m_global_timer = 0;
	unsigned long m_frame_count = 0;
	unsigned long m_engine_last_timestamp = 0;
	//unsigned long m_fuel_accum_time = 0;
	unsigned long m_mm3perhour = 0;

	uint8_t m_last_fuel_data = 0;
	uint32_t m_fuel_acc = 0;

	bool m_fuel_stat_available = false;

	void handle_ecm();
public:

	inline bool is_data_init(){
		// don't care about ECM
		return (m_data_available & 0b00011111) == 0b00011111;
	}

	inline uint8_t fuel_level_inst() {
		return (m_cluster_backup[4] & 0b11111110) >> 1;
	}
	inline int8_t external_temperature_inst() {
		return m_uch_general[4];
	}
	// if interrupt (INT) not used, mcp_int = 0
	Can_monitor(MCP_CAN& it);
	~Can_monitor();

	bool monitor();
	void get_instant_consumption(char buf[8]);
	char const* get_instant_consumption_unit();

	bool is_fuel_data_available(){
		bool ret = m_fuel_stat_available;
		m_fuel_stat_available = false;
		return ret;
	}

	inline float frames_per_seconds(){
		float result = float(m_frame_count) / (float)m_global_timer;
		return result * 0.001;
	}

	inline bool parking_light() {
		return m_uch_general[0] & 0b00000100;
	}

	inline bool low_beam_light() {
		return m_uch_general[0] & 0b00000010;
	}

	inline bool high_beam_light() {
		return m_uch_general[1] & 0b00001000;
	}

	inline bool fog_light() {
		return m_uch_general[1] & 0b00000001;
	}

	// engine coolant in degrees centigrad
	inline int8_t engine_coolant_temperature() {
		return m_uch_general[5] - 40;
	}

	// reverse gear engaged > true
	inline bool reverse_gear() {
		return m_uch_general[6] & 0b00010000;
	}

	inline long get_mm3(){
		return m_mm3perhour;
	}

	// driver door
	// open>true
	// close>false
	inline bool driver_door_open() {
		return ((m_uch_general[0] & 0b11111000) >> 3) & 0b00001;
	}

	inline bool passenger_door_open() {
		return ((m_uch_general[0] & 0b11111000) >> 3) & 0b00010;
	}

	// boot
	// open>true
	// close>false
	inline bool boot_open() {
		return ((m_uch_general[0] & 0b11111000) >> 3) & 0b10000;
	}

	// range
	// 0(low) <-> 8(max)
	inline uint8_t oil_level() {
		return (m_cluster_backup[3] & 0b00111100) >> 2;
	}

	inline uint8_t card_number() {
		return m_uch_general[7] & 0b1111;
	}

	inline bool must_sleep(){
		return !((m_uch_system[1] & 0b11) == 0b11);
	}

	inline bool ac_status(){
		return (m_uch_system[0] & 0b1) == 0b1;
	}

	inline int total_electric_power(){
		return (m_uch_system[3] & 0b11111) * 100;
	}

	inline int lux_solar_intensity(){
		return (m_uch_system[6]) * 1000;
	}

	inline cruise_control_ids cruise_control_status() {
		uint8_t status = (m_engine_general[5] & 0b1110000) >> 4;
		return (cruise_control_ids)status;
	}

	inline uint8_t engine_running() {
		return m_engine_general[5] == 2;
	}

	inline uint8_t cruise_control_speed() {
		return m_engine_general[4];
	}

	inline bool adac_switch(){
		return (m_uch_general[7] & 0b11);
	}

	inline unsigned long total_odometer() {
		unsigned long odo = 0;
		odo |= (unsigned long )m_cluster_backup[0] << 16;
		odo |= (unsigned long )m_cluster_backup[1] << 8;
		odo |= (unsigned long )m_cluster_backup[2];
		return odo;
	}

	inline uint16_t get_speed_raw(){
		unsigned long speed = 0;
		speed |= (unsigned long)m_brake_data[0] << 8;
		speed |= (unsigned long)m_brake_data[1];
		return speed;
	}

	inline uint16_t get_speed(){
	 return get_speed_raw() / 100;
	}

	inline unsigned long get_dm3perhour_times_100(){
		return m_mm3perhour;
	}

	inline uint8_t battery_voltage_raw(){
		return m_upc_data[2];
	}
	inline int freeRam() {
	  extern int __heap_start, *__brkval;
	  int v;
	  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
	}
};

#endif
