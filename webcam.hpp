#ifndef	__WEB_CAM_HPP__
#define	__WEB_CAM_HPP__

#include	<string>
#include	<tuple>
#include	<memory>
#include	<vector>

namespace EG {
	class Webcam {
	public:
		explicit Webcam(const std::string & dev);
		~Webcam();
		
		struct Size {
			int width;
			int height;
		};
		
		struct Point {
			int x;
			int y;
		};
		
		int
		brightness();
		
		void
		brightness(int value);
		
		// report device driver capabilities
		void
		reportCapability();
		
		// report system current webcam device
		// may be *NOT* this one
		void
		reportCurrentDevice();
		
		void
		reportBrightnessInfo();
		
		void
		reportFormats();
	
		void
		reportFourCCFrames(const char cc[4]);
		
		void
		reportFrameInterval(const char cc[4], Size frameSize);
		
		void
		start();
		
		void
		stop();
		
		void
		grab(unsigned char buf[480][640], int threshold = -1);
		
		std::tuple<bool, Point>
		search(int threshold = 128);
		
		void
		skip(int frameCount = 1);
		
		void
		initIgnore();
		
		void
		intersectIgnore();
		
		void
		exportLog();
	
	private:
		int _fd;
		const static int FRAME_SIZE = 640 * 480 * 2;
		const static int FRAME_BUF_COUNT = 2;
		std::unique_ptr<unsigned char[]>	_buf;
		
		bool _ignoreMap[480][640];
		
		// debug log images
		const static int LOG_SIZE = 640 * 480;
		const static int LOG_BUF_COUNT = 30 * 30; // 30 seconds, 30 fps
		std::vector<unsigned char>	_log;
		int	_seq; // log sequence
		std::vector<unsigned int>	_seqLog;
		
	private:
		Webcam() = delete;
		Webcam(const Webcam &) = delete;
		Webcam & operator=(const Webcam &) = delete;
	};
};

#endif //__WEB_CAM_HPP__


