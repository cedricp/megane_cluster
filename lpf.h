#pragma once

#include <limits.h>

template <class T>
class Value_cache {
	T m_val;
	bool m_val_changed;
public:
	Value_cache(T val = 0){
		m_val =0;
		m_val_changed = true;
	}

	T get_value(){
		return m_val;
	}

	bool set_value(T val){
		if (m_val != val){
			m_val_changed = true;
		}
		bool value_changed = m_val != val;
		m_val = val;
		return value_changed;
	}

	bool value_changed(){
		if (m_val_changed){
			m_val_changed = false;
			return true;
		}
		return false;
	}
};

class Lowpass_filter {
	unsigned long m_val, m_max;
	short m_pow;
	char m_init;
	bool m_value_changed;
public:
	Lowpass_filter(){
		m_val = 0;
		m_max = ULONG_MAX;
		m_pow = 16;
		m_init = 0;
		m_value_changed = false;
	}
	void set(unsigned long v){
		m_val = v;
		m_init = true;
	}
	void set_max(unsigned long max){

	}
	void set_pow(int pow){
		m_pow = pow;
	}
	bool filter(unsigned long val) {
		if (val > m_max){
			val = m_max;
		}
		if (!m_init){
			m_val = val << m_pow;
			m_init = 1;
			m_value_changed = true;
			return true;
		}
		unsigned long old_val = get_value();
		m_val = m_val - (m_val >> m_pow) + val;
		if (get_value() != old_val){
			m_value_changed = true;
			return true;
		}
		m_value_changed = false;
		return false;
	}

	unsigned long get_value(){
		return m_val >> m_pow;
	}

	bool value_changed(){
		return m_value_changed;
	}
};
