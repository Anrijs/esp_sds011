#ifndef _SDS011_H
#define _SDS011_H

#if ARDUINO >= 100
#include "Arduino.h"
#include "Print.h"
#else
#include "WProgram.h"
#endif

#include <Stream.h>

class Sds011 {
public:
	enum Report_mode {
		REPORT_ACTIVE = 0,
		REPORT_QUERY = 1
	};

	Sds011(Stream& out) : _out(out) {
	}

	bool device_info(String& firmware_version, uint16_t& device_id);
	bool set_data_reporting_mode(Report_mode mode);
	bool get_data_reporting_mode(Report_mode& mode);
	bool set_device_id(uint16_t device_id);
	bool set_sleep(bool sleep);
	bool get_sleep(bool& sleep);
	bool set_working_period(uint8_t minutes);
	bool get_working_period(uint8_t& minutes);
	bool query_data(int& pm25, int& pm10);
	bool query_data(int& pm25, int& pm10, int n);
	bool query_data_auto(int& pm25, int& pm10);
	bool query_data_auto(int& pm25, int& pm10, int n);
	bool timeout();
	bool crc_ok();

	void filter_data(int n, const int* pm25_table, const int* pm10_table, int& pm25, int& pm10);

protected:
	enum Command {
		CMD_DATA_REPORTING_MODE = 2,
		CMD_QUERY_DATA = 4,
		CMD_SET_DEVICE_ID = 5,
		CMD_SLEEP_AND_WORK = 6,
		CMD_FIRMWARE_VERSION = 7,
		CMD_WORKING_PERIOD = 8
	};

	void _send_cmd(enum Command cmd, const uint8_t* buf, uint8_t len);
	int _read_byte(long unsigned deadline = 0);
	String _buf_to_string();
	void _clear_responses();
	bool _read_response(enum Command cmd);

	Stream& _out;
	uint8_t _buf[19];
	bool _timeout = false;
};

template< class S > class Sds011Async : public Sds011 {
	static_assert(std::is_base_of<Stream, S>::value, "S must derive from Stream");
public:
	Sds011Async(S& out) : Sds011(out) {
	}

	void on_query_data_auto(std::function<void(int pm25, int pm10)> handler);

private:
	S & _get_out() { return static_cast<S&>(_out); }
	std::function<void(int pm25, int pm10)> query_data_auto_handler = 0;
};

template< class S > void Sds011Async< S >::on_query_data_auto(std::function<void(int pm25, int pm10)> handler) {
	if (!handler) { _get_out().onReceive(0); }
	query_data_auto_handler = handler;
	if (handler) {
		_get_out().onReceive([this](int avail) {
			int possibleMsgCnt = avail / 10;
			while (possibleMsgCnt--) {
				int pm25;
				int pm10;
				if (!query_data_auto(pm25, pm10)) { break; }
				query_data_auto_handler(pm25, pm10);
			}
		});
	}
}

#endif
