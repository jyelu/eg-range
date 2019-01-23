#include	"laser.hpp"
#include	<iostream>

const static int LASER_PIN = 8;

using namespace mraa;
using namespace EG;
using namespace std;

Laser::Laser() : _onOff(LASER_PIN), _spi(0)
{
	// setup on/off pin
	Result r = _onOff.dir(DIR_OUT_LOW); // out, and default low voltage
	if (r != SUCCESS) {
		cerr << "error setup GPIO dir" << endl;
		exit(1);
	}
	
	// setup spi
	r = _spi.mode(SPI_MODE1);
	if (r != SUCCESS) {
		cerr << "error setup SPI mode" << endl;
		exit(1);
	}
	
	// AD56x4R can support up to 50 MHz, and
	// Raspberry PI can support 250MHz / 2,
	// default set 32 MHz (which will downgrade to 250 / 8 = 31.25 MHz).
	//
	// however, people said it's impossible to reach 15 MHz or more...
	// http://raspberrypi.stackexchange.com/questions/699/what-spi-frequencies-does-raspberry-pi-support
	//
	// we let MRAA downgrade the speed automatically...
	r = _spi.frequency(32000000);
	
	// initial DAC
	init_dac();
}

Laser::~Laser()
{
	_onOff.write(0); // turn off
}

void
Laser::on()
{
	Result r = _onOff.write(1);
	if (r != SUCCESS) {
		cerr << "fail turn on laser" << endl;
		exit(1);
	}
}

void
Laser::off()
{
	Result r = _onOff.write(0);
	if (r != SUCCESS) {
		cerr << "fail turn on laser" << endl;
		exit(1);
	}
}

inline uint8_t
HIGH(int x)
{
	return (x >> 8) & 0xff;
}

inline uint8_t
LOW(int x)
{
	return x & 0xff;
}

void
Laser::move(int x, int y)
{
	const int M = 0x7fff;
	x &= M;
	y &= M;
	const int _x = M - x;
	const int _y = M - y;
	x += M;
	y += M;

	// write to DAC A
	set_dac_cmd(0x00, HIGH(x), LOW(x));
	send_dac_cmd();
	// write to DAC B
	set_dac_cmd(0x01, HIGH(_x), LOW(_x));
	send_dac_cmd();
	// write to DAC C
	set_dac_cmd(0x02, HIGH(_y), LOW(_y));
	send_dac_cmd();
	// write to DAC D, and update all
	set_dac_cmd(0x13, HIGH(y), LOW(y));
	send_dac_cmd();
}

void
Laser::send_dac_cmd()
{
	uint8_t * recv = _spi.write(_buf, 3);
	if (recv != NULL) {
		free(recv);
	} else {
		// fail
		cerr << "error send DAC command" << endl;
		exit(1);
	}
}

void
Laser::init_dac()
{
	// full reset DAC
	set_dac_cmd(0x28, 0x00, 0x01);
	send_dac_cmd();
	// enable internal voltage ref
	set_dac_cmd(0x38, 0x00, 0x01);
	send_dac_cmd();
	// enable all DAC channels
	set_dac_cmd(0x20, 0x00, 0x0f);
	send_dac_cmd();
	// enable software LDAC
	set_dac_cmd(0x30, 0x00, 0x00);
	send_dac_cmd();
}



