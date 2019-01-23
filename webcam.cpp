#include	"webcam.hpp"

#include	<opencv2/opencv.hpp>

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/ioctl.h>
#include	<fcntl.h>
#include	<unistd.h>

#include	<iostream>
#include	<ios>
#include	<stdio.h>
#include	<string.h>

#include	<linux/videodev2.h>

using namespace std;
using namespace EG;

Webcam::Webcam(const string & dev) :
	_buf(new unsigned char [FRAME_SIZE * FRAME_BUF_COUNT]),
	_seq(0)
{
	_fd = open(dev.c_str(), O_RDWR);
	if (_fd < 0) { // error when fd == -1
		perror("cannot open webcam device");
		exit(1);
	}
	
	// in order to increase frame rate
	// we need set exposure to lower value
	
	// adjuest exposure to manual
	struct v4l2_control control;
	memset(&control, 0, sizeof(control));
	control.id = V4L2_CID_EXPOSURE_AUTO;
	control.value = 1;
	if (0 != ioctl(_fd, VIDIOC_S_CTRL, &control)) {
		perror("fail to set manual exposure");
		exit(1);
	}
	// set exposure value
	memset(&control, 0, sizeof(control));
	control.id = V4L2_CID_EXPOSURE_ABSOLUTE;
	control.value = 15;
	if (0 != ioctl(_fd, VIDIOC_S_CTRL, &control)) {
		perror("fail to set exposure");
		exit(1);
	}
	
	// allocate log buffer
	_log.reserve(LOG_SIZE * LOG_BUF_COUNT);
}

Webcam::~Webcam()
{
	if (0 != close(_fd)) {
		perror("cannot close webcam device");
		// ignore this failure
	}
}

int
Webcam::brightness()
{
	struct v4l2_control control;
	memset(&control, 0, sizeof(control));
	control.id = V4L2_CID_BRIGHTNESS;
	if (0 == ioctl(_fd, VIDIOC_G_CTRL, &control)) {
		return control.value;
	}
	perror("fail to get brightness");
	exit(1);
}

void
Webcam::brightness(int value)
{
	struct v4l2_control control;
	memset(&control, 0, sizeof(control));
	control.id = V4L2_CID_BRIGHTNESS;
	control.value = value;
	if (0 == ioctl(_fd, VIDIOC_S_CTRL, &control)) {
		return;
	}
	perror("fail to set brightness");
	exit(1);
}

namespace {

#define STR(x) #x
#define TEST_AND_PRINT(var, v) do {\
	if (0 != (var & (V4L2_CAP_ ## v))) {\
		cout << " " << STR(V4L2_CAP_ ## v);\
	}\
} while (0)

void
printDeviceCapability(__u32 cap)
{
	// remark out the new version V4L2 attributes
	
	TEST_AND_PRINT(cap, VIDEO_CAPTURE);
	TEST_AND_PRINT(cap, VIDEO_CAPTURE_MPLANE);
	TEST_AND_PRINT(cap, VIDEO_OUTPUT);
	TEST_AND_PRINT(cap, VIDEO_OUTPUT_MPLANE);
//	TEST_AND_PRINT(cap, VIDEO_M2M);
//	TEST_AND_PRINT(cap, VIDEO_M2M_MPLANE);
	TEST_AND_PRINT(cap, VIDEO_OVERLAY);
	TEST_AND_PRINT(cap, VBI_CAPTURE);
	TEST_AND_PRINT(cap, VBI_OUTPUT);
	TEST_AND_PRINT(cap, SLICED_VBI_CAPTURE);
	TEST_AND_PRINT(cap, SLICED_VBI_OUTPUT);
	TEST_AND_PRINT(cap, RDS_CAPTURE);
	TEST_AND_PRINT(cap, VIDEO_OUTPUT_OVERLAY);
	TEST_AND_PRINT(cap, HW_FREQ_SEEK);
	TEST_AND_PRINT(cap, RDS_OUTPUT);
	TEST_AND_PRINT(cap, TUNER);
	TEST_AND_PRINT(cap, AUDIO);
	TEST_AND_PRINT(cap, RADIO);
	TEST_AND_PRINT(cap, MODULATOR);
//	TEST_AND_PRINT(cap, SDR_CAPTURE);
//	TEST_AND_PRINT(cap, EXT_PIX_FORMAT);
//	TEST_AND_PRINT(cap, SDR_OUTPUT);
	TEST_AND_PRINT(cap, READWRITE);
	TEST_AND_PRINT(cap, ASYNCIO);
	TEST_AND_PRINT(cap, STREAMING);
//	TEST_AND_PRINT(cap, DEVICE_CAPS);
}

#undef TEST_AND_PRINT
#undef	STR

};

void
Webcam::reportCapability()
{
	// retrive info
	struct v4l2_capability cap;
	if (-1 == ioctl(_fd, VIDIOC_QUERYCAP, &cap)) {
		perror("fail report driver capabilities");
		exit(1);
	}
	
	cout << "device driver capabilities:" << endl;
	cout << "\t[" << cap.driver;
	cout << "]<" << cap.card << ">" << endl;
	cout << "\ton bus: " << cap.bus_info << endl;
	const __u32 ver = cap.version;
	cout << "\tversion: " << ((ver >> 16) & 0xff)
		<< "." << ((ver >> 8) & 0xff)
		<< "." << (ver &0xff) << endl;
	
	cout << "\tphysical capabilities:";
	printDeviceCapability(cap.capabilities);
	cout << endl;
		
	// the new version v4l2
	//cout << "\tdevice capabilities:"
	//	<< printDeviceCapability(cap.device_caps) << endl;
	
}

void
Webcam::reportCurrentDevice()
{
	// get current selected webcam index
	int index;
	if (-1 == ioctl(_fd, VIDIOC_G_INPUT, &index)) {
		perror("fail to report current video index");
		exit(1);
	}
	cout << "current webcam: [" << index << "]" << endl;
	
	// retrive info of this index
	struct v4l2_input input;
	memset(&input, 0, sizeof(input));
	input.index = index;
	
	if (-1 == ioctl(_fd, VIDIOC_ENUMINPUT, &input)) {
		perror("fail enum dev to report");
		exit(1);
	}
	
	// print name
	cout << "\tname: " << input.name << endl;
	// print video type
	cout << "\ttype: ";
	switch (input.type) {
	case V4L2_INPUT_TYPE_TUNER:
		cout << "tuner (RF demodulator)";
		break;
	case V4L2_INPUT_TYPE_CAMERA:
		cout << "camera";
		break;
	default:
		cout << "unknown";
	}
	cout << endl;
	// print selectable audio set
	cout << "\tselectable audio set: " << hex << showbase
		<< input.audioset << endl;
	// print capabilities
	cout << "\tcapabilities: " << input.capabilities << endl
		<< dec;
	
	// not supported in C310
#if	0
	// print supported std
	cout << "\tsupported video standards: ";
	struct v4l2_standard standard;
	memset(&standard, 0, sizeof(standard));
	standard.index = 0;

	while (0 == ioctl(_fd, VIDIOC_ENUMSTD, &standard)) {
		if (standard.id & input.std) {
			cout << " " << standard.name;
		}
		++(standard.index);
	}

	// EINVAL when last one enum
	if (standard.index == 0 || errno != EINVAL) {
		perror("fail to enum device standards");
		exit(EXIT_FAILURE);
	}
#endif
}

void
Webcam::reportBrightnessInfo()
{
	struct v4l2_queryctrl queryctrl;
	memset(&queryctrl, 0, sizeof(queryctrl));
	queryctrl.id = V4L2_CID_BRIGHTNESS;
	if (-1 == ioctl(_fd, VIDIOC_QUERYCTRL, &queryctrl)) {
		perror("fail to reportBrightnessInfo");
		exit(1);
	}
	cout << "brightness: range(" << queryctrl.minimum << ", "
		<< queryctrl.maximum
		<< "), step: " << queryctrl.step
		<< ", default: " << queryctrl.default_value << endl;
}


namespace {

void
enumFrameSize(int fd, __u32 fourcc)
{
	struct v4l2_frmsizeenum fsize;
	for (int i = 0; ; ++i) {
		memset(&fsize, 0, sizeof(fsize));
		fsize.index = i;
		fsize.pixel_format = fourcc;
		if (0 != ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsize)) {
			// EINVAL when last one enum
			if (errno == EINVAL) {
				cout << endl;
				return;
			} else {
				perror("error enum frame sizes");
				exit(1);
			}
		}
		
		const struct v4l2_frmsize_stepwise & step = fsize.stepwise;
		switch(fsize.type) {
		case V4L2_FRMSIZE_TYPE_DISCRETE:
			cout << " " << fsize.discrete.width
				<< "x" << fsize.discrete.height;
			break;
		case V4L2_FRMSIZE_TYPE_CONTINUOUS:
			cout << "CONT!!!!" << endl;
			// fall through
		case V4L2_FRMSIZE_TYPE_STEPWISE:
			cout << " width: (" <<  step.min_width
				<<  ", " << step.max_width
				<<  ") +/- " << step.step_width
				<< "height: (" << step.min_height
				<<  ", " << step.max_height
				<<  ") +/- " << step.step_height;
			break;
		default:
			cout << "\t\tunknown frame size" << endl;
		}
	}
}

};

void
Webcam::reportFormats()
{
	struct v4l2_fmtdesc fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.index = 0;
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	cout << "formats: " << endl;
	
	while (0 == ioctl(_fd, VIDIOC_ENUM_FMT, &fmt)) {
		const __u32 fourcc = fmt.pixelformat;
#define	PART(x, shift)	((char)((x >> shift) & 0xff))
		cout << "\t[" << PART(fourcc, 0) << PART(fourcc, 8)
			<< PART(fourcc, 16) << PART(fourcc, 24);
#undef		PART
		cout << "] " << (const char *) fmt.description << endl;

		++(fmt.index);
	}

	// EINVAL when last one enum
	if (fmt.index == 0 || errno != EINVAL) {
		perror("fail to enum device standards");
		exit(EXIT_FAILURE);
	}
}

namespace {

__u32
fromStr(const char cc[4])
{
	__u8 ucc[4];
	memcpy(ucc, cc, 4);
	
	return ucc[0] | ucc[1] << 8 | ucc[2] << 16 | ucc[3] << 24;
}

double
fromFract(const v4l2_fract & f)
{
	return 1.0 * f.numerator / f.denominator;
}

};

void
Webcam::reportFourCCFrames(const char cc[4])
{
	enumFrameSize(_fd, fromStr(cc));
}

void
Webcam::reportFrameInterval(const char cc[4], Size frameSize)
{
	const __u32 fourcc = fromStr(cc);
	struct v4l2_frmivalenum param;
	for (int i = 0; ; ++i) {
		memset(&param, 0, sizeof(param));
		param.index = i;
		param.pixel_format = fourcc;
		param.width = frameSize.width;
		param.height = frameSize.height;
		if (0 != ioctl(_fd, VIDIOC_ENUM_FRAMEINTERVALS, &param)) {
			// EINVAL when last one enum
			if (errno == EINVAL) {
				cout << endl;
				return;
			} else {
				perror("error enum frame intervals");
				exit(1);
			}
		}
		
		switch(param.type) {
		case V4L2_FRMIVAL_TYPE_DISCRETE:
		{
			const v4l2_fract & discrete = param.discrete;
			const double sec = fromFract(param.discrete);
			cout << " " << 1000.0 * sec << "ms("
				<< 1.0/sec << ")";
		}
			break;
		case V4L2_FRMIVAL_TYPE_CONTINUOUS:
			cout << "CONT!!!!" << endl;
			// fall through
		case V4L2_FRMIVAL_TYPE_STEPWISE:
		{
			const v4l2_frmival_stepwise & step = param.stepwise;
			cout << " step(" <<  fromFract(step.min)
				<<  ", " << fromFract(step.max)
				<<  ")+/-" << fromFract(step.step);
		}
			break;
		default:
			cout << "\t\tunknown frame interval" << endl;
		}
	}
}

void
Webcam::start()
{
	// first, ask driver using user-ptr
	// driver must support 'STREAMING' capability
	struct v4l2_requestbuffers param;
	memset(&param, 0, sizeof(param));
	param.count = FRAME_BUF_COUNT;
	param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	param.memory = V4L2_MEMORY_USERPTR;
	if (0 != ioctl(_fd, VIDIOC_REQBUFS, &param)) {
		perror("cannot set streamming");
		exit(1);
	}
	
	// second, register buffers to driver
	struct v4l2_buffer buffer;
	for (int i = 0; i < FRAME_BUF_COUNT; ++i) {
		memset(&buffer, 0, sizeof(buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.index = i;
		buffer.memory = V4L2_MEMORY_USERPTR;
		buffer.m.userptr = (unsigned long)&(_buf[FRAME_SIZE * i]);
		buffer.length = FRAME_SIZE;
		if (0 != ioctl(_fd, VIDIOC_QBUF, &buffer)) {
			perror("cannot register buf");
			exit(1);
		}
	}
	
	// finally, start streaming
	const int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 != ioctl(_fd, VIDIOC_STREAMON, &type)) {
		perror("fail starting streaming");
		exit(1);
	}
}

void
Webcam::stop()
{
	const int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 != ioctl(_fd, VIDIOC_STREAMOFF, &type)) {
		perror("fail stoping streaming");
		exit(1);
	}
}

void
Webcam::grab(unsigned char buf[480][640], int threshold)
{
	struct v4l2_buffer buffer;
	for (;;) {
		// keep retry if driver tell us grabing failure
		memset(&buffer, 0, sizeof(buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_USERPTR;
		if (0 != ioctl(_fd, VIDIOC_DQBUF, &buffer)) {
			perror("cannot grab buf");
			exit(1);
		}
		if (buffer.flags & V4L2_BUF_FLAG_ERROR) {
			// if fail
			// put back buffer
			if (0 != ioctl(_fd, VIDIOC_QBUF, &buffer)) {
				perror("fail register buf");
				exit(1);
			}
			continue; // try again
		}
		
		// copy content
		const __u8 * p = (const __u8 *) (buffer.m.userptr);
		__u8 * dst = &(buf[0][0]);
		for (int i = 0; i < 480 * 640; ++i) {
			if (threshold >= 0) {
				if (*p > threshold) {
					*dst = 255;
				} else {
					*dst = 0;
				}
			} else {
				*dst = *p;
			}
			++dst;
			++p; ++p; // copy 'Y' only, discards 'U/V'
		}
		
		// put back buffer
		if (0 != ioctl(_fd, VIDIOC_QBUF, &buffer)) {
			perror("fail register buf back");
			exit(1);
		}
		return;
	}
}

// if not found, return (-1, -1)
tuple <bool, Webcam::Point>
Webcam::search(int threshold)
{
	struct v4l2_buffer buffer;
	for (;;) {
		// keep retry if driver tell us grabing failure
		memset(&buffer, 0, sizeof(buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_USERPTR;
		if (0 != ioctl(_fd, VIDIOC_DQBUF, &buffer)) {
			perror("cannot search buf");
			exit(1);
		}
		if (buffer.flags & V4L2_BUF_FLAG_ERROR) {
			// if fail
			// put back buffer
			if (0 != ioctl(_fd, VIDIOC_QBUF, &buffer)) {
				perror("fail register buf (search)");
				exit(1);
			}
			continue; // try again
		}
		
		const __u8 (*d)[1280] = (const __u8 (*)[1280]) (buffer.m.userptr);
		bool found = false;
		Point upperLeft{ 640, 480 }, lowerRight {-1, -1};
		for (int y = 0; y < 480; ++y) {
			for (int x = 0; x < 640; ++x) {
				const __u8 luma = d[y][2 * x];
				if (luma > threshold && !_ignoreMap[y][x]) {
					found = true;
					if (x > lowerRight.x)
						lowerRight.x = x;
					if (x < upperLeft.x)
						upperLeft.x = x;
					if (y > lowerRight.y)
						lowerRight.y = y;
					if (y < upperLeft.y)
						upperLeft.y = y;
				}
				if (_seq < LOG_BUF_COUNT) {
					_log.push_back(luma);
				}
				
			}
		}
		if (_seq < LOG_BUF_COUNT) {
			++_seq;
			_seqLog.push_back(buffer.sequence);
		}
		
		// put back buffer
		if (0 != ioctl(_fd, VIDIOC_QBUF, &buffer)) {
			perror("fail register buf back (search)");
			exit(1);
		}
		if (!found)
			return make_tuple(false, Point{-1,-1});
		
		return make_tuple(true, Point{
			(upperLeft.x + lowerRight.x) / 2,
			(upperLeft.y + lowerRight.y) / 2 });
	}
}

void
Webcam::skip(int frameCount)
{
	if (frameCount <= 0) {
		return;
	}
	
	struct v4l2_buffer buffer;
	for (;;) {
		// keep retry if driver tell us grabing failure
		memset(&buffer, 0, sizeof(buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_USERPTR;
		if (0 != ioctl(_fd, VIDIOC_DQBUF, &buffer)) {
			perror("cannot search buf");
			exit(1);
		}
		if (buffer.flags & V4L2_BUF_FLAG_ERROR) {
			// if fail
			// put back buffer
			if (0 != ioctl(_fd, VIDIOC_QBUF, &buffer)) {
				perror("fail register buf (search)");
				exit(1);
			}
			continue; // try again
		}
		
		// put back buffer
		if (0 != ioctl(_fd, VIDIOC_QBUF, &buffer)) {
			perror("fail register buf back (search)");
			exit(1);
		}
		
		--frameCount;
		if (frameCount <= 0) {
			return;
		}
	}
}

void
Webcam::initIgnore()
{
	struct v4l2_buffer buffer;
	for (;;) {
		// keep retry if driver tell us grabing failure
		memset(&buffer, 0, sizeof(buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_USERPTR;
		if (0 != ioctl(_fd, VIDIOC_DQBUF, &buffer)) {
			perror("cannot search buf");
			exit(1);
		}
		if (buffer.flags & V4L2_BUF_FLAG_ERROR) {
			// if fail
			// put back buffer
			if (0 != ioctl(_fd, VIDIOC_QBUF, &buffer)) {
				perror("fail register buf (search)");
				exit(1);
			}
			continue; // try again
		}
		
		const __u8 (*d)[1280] = (const __u8 (*)[1280]) (buffer.m.userptr);
		for (int y = 0; y < 480; ++y) {
			for (int x = 0; x < 640; ++x) {
				const __u8 luma = d[y][2 * x];
				if (luma >= 80) {
					_ignoreMap[y][x] = true;
				} else {
					_ignoreMap[y][x] = false;
				}
				
			}
		}
		
		// put back buffer
		if (0 != ioctl(_fd, VIDIOC_QBUF, &buffer)) {
			perror("fail register buf back (search)");
			exit(1);
		}
		return;
	}
}

void
Webcam::intersectIgnore()
{
	struct v4l2_buffer buffer;
	for (;;) {
		// keep retry if driver tell us grabing failure
		memset(&buffer, 0, sizeof(buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_USERPTR;
		if (0 != ioctl(_fd, VIDIOC_DQBUF, &buffer)) {
			perror("cannot search buf");
			exit(1);
		}
		if (buffer.flags & V4L2_BUF_FLAG_ERROR) {
			// if fail
			// put back buffer
			if (0 != ioctl(_fd, VIDIOC_QBUF, &buffer)) {
				perror("fail register buf (search)");
				exit(1);
			}
			continue; // try again
		}
		
		const __u8 (*d)[1280] = (const __u8 (*)[1280]) (buffer.m.userptr);
		for (int y = 0; y < 480; ++y) {
			for (int x = 0; x < 640; ++x) {
				const __u8 luma = d[y][2 * x];
				if (_ignoreMap[y][x]) {
					if (luma < 80) {
						_ignoreMap[y][x] = false;
					}
				}
				
			}
		}
		
		// put back buffer
		if (0 != ioctl(_fd, VIDIOC_QBUF, &buffer)) {
			perror("fail register buf back (search)");
			exit(1);
		}
		return;
	}
}

void
Webcam::exportLog()
{
	using namespace cv;
	cv::Size size(640.0, 480.0);
	unsigned char buf[480][640];
	Mat grayFrame(size, CV_8UC1, buf);
	unsigned char * p = (unsigned char *)(&(buf[0][0]));
	
	auto it = _log.cbegin();
	for (auto i : _seqLog) {
		std::string filename { "./log/" };
		filename += std::to_string(i);
		filename += ".png";
		std::copy_n(it, LOG_SIZE, p);
		it += LOG_SIZE;
		imwrite(filename, grayFrame);
	}
	
	// export mask
	for (int y = 0; y < 480; ++y) {
		for (int x = 0; x < 640; ++x) {
			if (_ignoreMap[y][x]) {
				buf[y][x] = 255;
			} else {
				buf[y][x] = 0;
			}
		}
	}
	imwrite("./log/mask.png", grayFrame);
}

void
webcam_main()
{
	EG::Webcam webcam { "/dev/video0" };
	webcam.reportCurrentDevice();
	webcam.reportCapability();
	webcam.reportBrightnessInfo();
	webcam.reportFormats();
	
	cout << endl << "enum the YUYV format frame sizes:";
	webcam.reportFourCCFrames({"YUYV"});
	
	cout << endl << "enum the YUYV 640x480 frame intervals: ";
	webcam.reportFrameInterval({ "YUYV" }, { 640, 480 });
	
	webcam.brightness(203);
	cout << "current brightness: " << webcam.brightness() << endl;
}

