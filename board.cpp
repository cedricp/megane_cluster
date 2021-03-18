#include "board.h"

#include <avr/pgmspace.h>
#include "bitmaps.h"

#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8

void Widget::draw(SSD1351& display, String value, int text_size, int color) {
	display.fillRect(m_x, m_y, m_w, m_h, SSD1351::BLACK);
	display.setTextSize(text_size);
	if (m_orient == Widget::HORIZONTAL){
		const int half_width = m_w / 2;
		display.setCursor(m_x +1, m_y +1);
		display.setTextColor(SSD1351::BLACK);
		display.print(m_title);
		display.setCursor(m_x +1 + half_width, m_y +1);
		display.setTextColor(color);
		display.print(value);
	} else {
		if (m_title.length()){
			const int half_height = m_h / 2;
			int title_x_pos = (m_w - (m_title.length() * CHAR_WIDTH * text_size)) / 2;
			if (title_x_pos < 0)
				title_x_pos = 1;
			int val_x_pos = (m_w - (value.length() * CHAR_WIDTH * text_size)) / 2;
			if (val_x_pos < 0)
				val_x_pos = 1;
			display.setCursor(m_x + title_x_pos, m_y +1);
			display.setTextColor(SSD1351::WHITE);
			display.print(m_title);
			display.setCursor(m_x +1 + val_x_pos, m_y + half_height + 1);
			display.setTextColor(color);
			display.print(value);
		} else {
			int val_x_pos = (m_w - (value.length() * CHAR_WIDTH * text_size)) / 2;
			if (val_x_pos < 1)
				val_x_pos = 1;
			display.setTextSize(text_size);
			display.setCursor(m_x +1 + val_x_pos, m_y + 1);
			display.setTextColor(color);
			display.print(value);
		}
	}
}

Board::Board(SSD1351& lcd, Can_monitor& monitor) : m_display(lcd), m_monitor(monitor)
{
	m_speed.set_pow(12);
	m_fuel_level.set_max(90);
	m_battery_volt.set_pow(18);
}

Board::~Board()
{

}

void
Board::init() {
	m_display.fillScreen(SSD1351::WHITE);
	set_external_temp();
	set_mileage();
	set_battery();
	set_coolant();
	set_oil_level();
	set_speed();
	set_consumption();
	set_cc_status();
	set_fuel();
	m_display.fillRect(0, 75, 128, 53, SSD1351::BLACK);
	set_power();
	set_aircon_status();
	set_door_status(true);
}

void
Board::set_door_status(bool init)
{
	bool changed = init;
	changed |= m_boot != m_monitor.boot_open();
	changed |= m_left_door != m_monitor.driver_door_open();
	changed |= m_right_door != m_monitor.passenger_door_open();

	m_boot = m_monitor.boot_open();
	m_left_door = m_monitor.driver_door_open();
	m_right_door = m_monitor.passenger_door_open();

	if (changed){
		m_display.fillRect(44, 76, 40, 53, SSD1351::BLACK);
		m_display.drawRGBBitmap(50, 76, car_small, 28, 53);
		if (m_boot){
			m_display.drawRGBBitmap(44, 76+40, car_small_open, 40, 33, 0, 40, 20, 33);
		}
		if (m_left_door){
			m_display.drawRGBBitmap(44, 76+20, car_small_open, 40, 33, 0, 20, 0, 20);
		}
		if (m_right_door){
			m_display.drawRGBBitmap(44, 76+20, car_small_open, 40, 33, 26, 40, 0, 20);
		}
	}
}

void
Board::set_aircon_status()
{
	if (m_aircon.get_value()){
		m_display.drawRGBBitmap(112, 112, aircon_icon, 16, 16);
	} else {
		m_display.fillRect(112, 112, 16, 16, SSD1351::BLACK);
	}
}

void
Board::set_external_temp() {
	char buff[10];
	int color;
	int temp = m_ext_temp.get_value() - 40;
	if (temp  > 30){
		color = SSD1351::YELLOW;
	} else if (temp  > 3){
		color = SSD1351::GREEN;
	} else {
		color = SSD1351::CYAN;
	}
	snprintf(buff, 10, "%d%c", temp, 247);
	buff[3] = 0;
	//Widget w("", 0, 0, CHAR_WIDTH * 6, CHAR_HEIGHT * 2 + 1 , Widget::VERTICAL);
	//w.draw(m_display, buff, 2, color);
	m_display.fillRect(0, 0, CHAR_WIDTH * 6, CHAR_HEIGHT * 2 + 1, SSD1351::BLACK);
	m_display.setCursor(0,0);
	m_display.setTextSize(2);
	m_display.setTextColor(color);
	m_display.print(buff);
}

void
Board::set_power() {
	char buff[10];
	int color;
	int power = m_power.get_value();
	if (power  > 1000){
		color = SSD1351::YELLOW;
	} else if (power  > 2000){
		color = SSD1351::GREEN;
	} else {
		color = SSD1351::BLUE;
	}
	snprintf(buff, 10, "%dW", power);
	Widget w("POWER", 0, 75, CHAR_WIDTH * 6, CHAR_HEIGHT * 2 + 1 , Widget::VERTICAL);
	w.draw(m_display, buff, 1, color);
}

void
Board::set_mileage() {
	char buffer[10];
	m_display.setTextSize(2);
	m_display.fillRect(37,0, 128-37, CHAR_HEIGHT*2 + 1, SSD1351::BLACK);
	m_display.setTextColor(SSD1351::MAGENTA);
	m_display.setCursor(41,0);
	sprintf(buffer, "%06lu", m_kilometers);
	m_display.print(buffer);
	m_display.setTextSize(1);
	m_display.print("Km");
}

void
Board::set_battery() {
	float batt = float(m_battery_volt.get_value()) * 0.0625;
	char buff[7];
	snprintf(buff, 7, "%d.%d V", (int)batt, int(batt*10)%10);
	int color = SSD1351::RED;
	if (batt > 14.5){
		color = SSD1351::RED;
	} else if (batt > 12.2){
		color = SSD1351::GREEN;
	} else if (batt > 12.0){
		color = SSD1351::YELLOW;
	} else {
		color = SSD1351::RED;
	}
	Widget w("BATT", 0, 18, 41, 17, Widget::VERTICAL);
	w.draw(m_display, buff, 1, color);
}

void
Board::set_coolant() {
	char buff[7];
	snprintf(buff, 7, "%d%cC", m_coolant.get_value(), (char)247);

	int color = SSD1351::RED;
	if (m_coolant.get_value() > 100){
		color = SSD1351::RED;
	} else if (m_coolant.get_value() > 70){
		color = SSD1351::GREEN;
	} else if (m_coolant.get_value() > 40) {
		color = SSD1351::YELLOW;
	}
	Widget w("COOLANT", 42, 18, CHAR_WIDTH * 7 + 1, 17, Widget::VERTICAL);
	w.draw(m_display, buff, 1, color);
}

void
Board::set_oil_level() {
	char buff[7];
	int level = m_oil_level.get_value();
	buff[6] = 0;
	for (int i = 0; i < 6; ++i){
		if (level > i){
			buff[i] = '*';
		} else {
			buff[i] = '-';
		}
	}

	int color = SSD1351::RED;
	if (level > 4){
		color = SSD1351::GREEN;
	} else if (level > 2){
		color = SSD1351::YELLOW;
	}

	Widget w("OIL", 86, 18, CHAR_WIDTH * 7, 17, Widget::VERTICAL);
	w.draw(m_display, buff, 1, color);
}

void
Board::set_speed() {
	char buff[4];
	snprintf(buff, 7, "%03d", m_speed.get_value());
	m_display.setTextColor(SSD1351::WHITE, SSD1351::BLACK);
	m_display.fillRect(0, 36, 72, 38, SSD1351::BLACK);
	m_display.setTextSize(4);
	m_display.setCursor(0, 41);
	m_display.print(buff);
}

void
Board::set_consumption()
{
	String label = m_monitor.get_instant_consumption();
	int margin = (55 - label.length() * CHAR_WIDTH) / 2;
	m_display.fillRect(73, 36, 55, 9, SSD1351::BLACK);
	m_display.setTextSize(1);
	m_display.setCursor(73 + margin, 37);
	m_display.setTextColor(SSD1351::WHITE, SSD1351::BLACK);
	m_display.print(label);
}

void
Board::set_cc_status()
{
	String buff;
	int color = SSD1351::GREEN;
	bool free = false;
	if (m_cruise_control_mode == CC_ACTIVE){
		buff = "CC:";
		color = SSD1351::GREEN;
	} else if (m_cruise_control_mode == CC_INACTIVE){
		buff = "CC:";
		color = SSD1351::YELLOW;
	} else if (m_cruise_control_mode == LIMITER_ACTIVE){
		buff = "SL:";
		color = SSD1351::GREEN;
	} else if (m_cruise_control_mode == LIMITER_INACTIVE){
		buff = "SL:";
		color = SSD1351::YELLOW;
	} else {
		buff = "   FREE  ";
		free = true;
	}
	if (!free){
		if (m_cruise_control_speed.get_value() >= 254){
			buff += "N/A";
		} else if (m_cruise_control_mode != OFF){
			buff += m_cruise_control_speed.get_value();
			buff += "Kmh";
		}
	}
	m_display.setTextSize(1);
	m_display.fillRect(73, 46, 55, 9, SSD1351::BLACK);
	m_display.setCursor(74, 47);
	m_display.setTextColor(color, SSD1351::BLACK);
	m_display.print(buff);
}

void
Board::set_fuel()
{
	char buff[5];
	snprintf(buff, 7, " %02dL", m_fuel_level.get_value());
	m_display.fillRect(73, 56, 55, 18, SSD1351::BLACK);
	m_display.setCursor(73, 58);
	m_display.setTextSize(2);
	if(m_fuel_level.get_value() > 15){
		m_display.setTextColor(SSD1351::GREEN, SSD1351::BLACK);
	} else if (m_fuel_level.get_value() > 7){
		m_display.setTextColor(SSD1351::YELLOW, SSD1351::BLACK);
	} else {
		m_display.setTextColor(SSD1351::RED, SSD1351::BLACK);
	}
	m_display.print(buff);
}

void
Board::update(bool init) {
	if (init){
		m_display.fillRect(0,0, 128, 128, SSD1351::WHITE);
	}
	unsigned long kilometers = m_monitor.total_odometer();
	int ext_temp = m_monitor.external_temperature_inst();
	int coolant = m_monitor.engine_coolant_temperature();
	int oil = m_monitor.oil_level();
	int speed = m_monitor.get_speed();
	cruise_control_ids cc_mode = m_monitor.cruise_control_status();
	int cc_speed = m_monitor.cruise_control_speed();
	int fuel_level = m_monitor.fuel_level_inst();
	float battery = m_monitor.battery_voltage_raw();
	//long mm3 = m_monitor.get_mm3();
	int power = m_monitor.total_electric_power();
	int aircon = m_monitor.ac_status();

	unsigned long current_time = millis();

	if (m_power.filter(power) || init){
		set_power();
	}
	if (m_kilometers.set_value(kilometers) || init){
		set_mileage();
	}
	if (m_ext_temp.filter(ext_temp) || init){
		set_external_temp();
	}
	if (m_battery_volt.filter(battery) || init){
		set_battery();
	}
	if (m_fuel_level.filter(fuel_level) || init){
		set_fuel();
	}
	if (m_oil_level.set_value(oil) || init){
		set_oil_level();
	}
	if (m_coolant.set_value(coolant) || init){
		set_coolant();
	}

	if (m_speed.filter(speed) || init){
		set_speed();
	}
	if (m_aircon.set_value(aircon) || init){
		set_aircon_status();
	}
	if (m_monitor.is_fuel_data_available() || init){
		m_fuel_stat_time = current_time;
		set_consumption();
	}
	if (cc_mode != m_cruise_control_mode || m_cruise_control_speed.set_value(cc_speed) || init){
		m_cruise_control_mode = cc_mode;
		set_cc_status();
	}
	set_door_status(init);
//	if (m_monitor.must_sleep()){
//		m_display.fillRect(70,70,30,30, SSD1351::RED);
//	} else {
//		m_display.fillRect(70,70,30,30, SSD1351::GREEN);
//	}
//	m_display.setTextColor(SSD1351::GREEN);
//	m_display.setCursor(70,70);
//	m_display.print(m_kilometers);
}

