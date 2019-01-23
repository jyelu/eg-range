#include	"webcam.hpp"
#include	"laser.hpp"

#include	<iostream>
#include	<thread>

using namespace std;

inline void
sleep_ms(int ms)
{
	this_thread::sleep_for(chrono::milliseconds(ms));
}

inline void
sleep_us(int us)
{
	this_thread::sleep_for(chrono::microseconds(us));
}

int
main()
{
	//extern void webcam_main();
	//webcam_main();
	
	EG::Webcam webcam { "/dev/video0" };
	webcam.brightness(127);
	
	EG::Laser laser;
	laser.move(0, 0);
	laser.off();
	
	typedef EG::Webcam::Point Point;
	Point p;
	bool found;
	
	webcam.start();
	// setup ignore map (the original point)
	// we turn on and move laser to a coner
	laser.move(20000, 0);
	laser.on();
	sleep_ms(200); // wait laser point stable
	// skip 2 frames
	webcam.skip(2);
	// initIgnore will setup all bright zones
	webcam.initIgnore();
	// move to another coner
	laser.move(0, 20000);
	sleep_ms(200); // wait laser point stable
	// skip 2 frames
	webcam.skip(2);
	// intersectIgnore discards all zones which are dark now
	webcam.intersectIgnore();
	
	// turn off
	laser.off();
	sleep_ms(1);
	// skip 2 frames
	webcam.skip(2);
	
	// alert if found sparks
	do {
		tie(found, p) = webcam.search();
		if (found) {
			cerr << " UFO!";
		}
	} while (found);
	
	laser.on();
	Point mesh[9][9];
	for (int y = 0; y < 9; ++y) {
		for (int x = 0; x < 9; ++x) {
			laser.move(2500 * x, 2500 * y);
			sleep_ms(1);
			webcam.skip(); // skip a frame
			Point & p = mesh[y][x];
			do {
				tie(found, p) = webcam.search();
				if (!found) {
					cerr << " not found!";
				}
			} while (!found);
			
			cout << p.x << "," << p.y << " ";
		}
		cout << endl;
	}
	
	laser.off();
	webcam.stop();


	webcam.exportLog();
	cout << "done" << endl;
	
	return 0;
}

