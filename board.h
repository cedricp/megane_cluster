#pragma once

#include "SSD1351.h"
#include "monitor.h"
#include "lpf.h"


struct tireInfo
{
	uint8_t temp, pressure = 99;
	bool leak = false, nosignal = false, lowbatt = false;
	bool up2date = false;
	uint8_t id[4];
};

class Widget{
public:
	enum ORIENT{
		VERTICAL,
		HORIZONTAL
	};
	Widget(char const* title, int x, int y, int w, int h, ORIENT o){
		strcpy(m_title, title);
		m_x = x;
		m_y = y;
		m_w = w;
		m_h = h;
		m_orient = o;
	}

	void draw(SSD1351& display, char const* value, int text_size, int color);

private:
	char m_title[16];
	ORIENT m_orient;
	int m_x, m_y, m_w, m_h;
};

class Board {
	SSD1351& m_display;
	Can_monitor& m_monitor;
	Lowpass_filter m_ext_temp, m_fuel_level, m_battery_volt, m_speed;
	Value_cache<unsigned long> m_kilometers;
	Value_cache<int> m_coolant, m_oil_level, m_cruise_control_speed;
	Value_cache<bool> m_aircon;
	cruise_control_ids m_cruise_control_mode = OFF;
	//unsigned long m_fuel_stat_time = 0;
	bool m_boot = false;
	bool  m_left_door = false;
	bool  m_right_door = false;
	tireInfo m_tires[4];

	void set_external_temp();
	void set_mileage();
	void set_cc_status();
	void set_battery();
	void set_coolant();
	void set_oil_level();
	void set_speed(bool init = false);
	void set_consumption(bool init=false);
	void set_fuel();
	void set_door_status(bool init = false);
	void set_aircon_status();
	void set_tires_info();

public:
	Board(SSD1351& lcd, Can_monitor& monitor);
	~Board();

	void init();
	void update(bool init = false);
	tireInfo& getTireInfo(int pos){
		return m_tires[pos];
	}
};
