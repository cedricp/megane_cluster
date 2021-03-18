#pragma once

#include "SSD1351.h"
#include "monitor.h"
#include "lpf.h"

class Widget{
public:
	enum ORIENT{
		VERTICAL,
		HORIZONTAL
	};
	Widget(String title, int x, int y, int w, int h, ORIENT o){
		m_title = title;
		m_x = x;
		m_y = y;
		m_w = w;
		m_h = h;
		m_orient = o;
	}

	void draw(SSD1351& display, String value, int text_size, int color);

private:
	String m_title;
	ORIENT m_orient;
	int m_x, m_y, m_w, m_h;
};

class Board {
	SSD1351& m_display;
	Can_monitor& m_monitor;
	Lowpass_filter m_ext_temp, m_fuel_level, m_battery_volt, m_speed, m_power;
	Value_cache<unsigned long> m_kilometers;
	Value_cache<int> m_coolant, m_oil_level, m_cruise_control_speed;
	Value_cache<bool> m_aircon;
	cruise_control_ids m_cruise_control_mode = OFF;
	unsigned long m_fuel_stat_time = 0;
	bool m_boot = false;
	bool  m_left_door = false;
	bool  m_right_door = false;

	void set_external_temp();
	void set_mileage();
	void set_power();
	void set_cc_status();
	void set_battery();
	void set_coolant();
	void set_oil_level();
	void set_speed();
	void set_consumption();
	void set_fuel();
	void set_door_status(bool init = false);
	void set_aircon_status();

public:
	Board(SSD1351& lcd, Can_monitor& monitor);
	~Board();

	void init();
	void update(bool init = false);
};
