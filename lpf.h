#pragma once

#include <limits.h>

template <class T>
class Value_cache {
	T m_val;
	bool m_val_changed;
public:
	Value_cache(T val = 0){
		m_val =0;
	}

	T get_value(){
		return m_val;
	}

	bool set_value(T val){
		bool value_changed = m_val != val;
		m_val = val;
		return value_changed;
	}
};

class Lowpass_filter {
	unsigned long m_val, m_max;
	short m_pow;
	bool m_init;
public:
	Lowpass_filter(){
		m_val = 0;
		m_max = ULONG_MAX;
		m_pow = 8;
		m_init = false;
	}
	void set(unsigned long v){
		m_val = v;
		m_init = true;
	}
	void set_max(unsigned long max){
		m_max = max;
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
			m_init = true;
			return true;
		}
		unsigned long old_val = get_value();
		m_val = m_val - (m_val >> m_pow) + val;
		if (get_value() != old_val){
			return true;
		}
		return false;
	}

	unsigned long get_value(){
		return m_val >> m_pow;
	}
};
