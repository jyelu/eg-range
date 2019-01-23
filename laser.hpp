#ifndef	__LASER_HPP__
#define	__LASER_HPP__

#include	<mraa.h>

#include	"mraa/gpio.hpp"
#include	"mraa/spi.hpp"

namespace EG {
	
	class Laser {
	public:
		Laser();
		~Laser();
		
		void
		on();
		
		void
		off();
		
		void
		move(int x, int y);
		
	private:
		mraa::Gpio	_onOff;
		mraa::Spi	_spi;
		
		uint8_t		_buf[3];
		
	private:
		void
		send_dac_cmd();
		
		inline void
		set_dac_cmd(uint8_t cmd1, uint8_t cmd2, uint8_t cmd3)
		{
			_buf[0] = cmd1; _buf[1] = cmd2; _buf[2] = cmd3;
		}
		
		void
		init_dac();
		
	private:
		Laser & operator=(const Laser &) = delete;
		Laser(const Laser &) = delete;
	};
};

#endif // __LASER_HPP__

