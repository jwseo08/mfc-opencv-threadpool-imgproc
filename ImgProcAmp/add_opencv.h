#pragma once

#include "opencv2/opencv.hpp"

using namespace cv;

#ifndef _OPENCV_STATIC

// dynamic link
//---------------------------------------------------------
#ifdef _DEBUG
#pragma comment(lib, "opencv_world4100d.lib")
#else
#pragma comment(lib, "opencv_world4100.lib")
#endif
//---------------------------------------------------------

#else

// static link
//---------------------------------------------------------
#ifdef _DEBUG

#pragma comment(lib, "aded.lib")
#pragma comment(lib, "IlmImfd.lib")
#pragma comment(lib, "ippicvmt.lib")
#pragma comment(lib, "ippiwd.lib")
#pragma comment(lib, "ittnotifyd.lib")
#pragma comment(lib, "libjpeg-turbod.lib")
#pragma comment(lib, "libopenjp2d.lib")
#pragma comment(lib, "libpngd.lib")
#pragma comment(lib, "libprotobufd.lib")
#pragma comment(lib, "libtiffd.lib")
#pragma comment(lib, "libwebpd.lib")
#pragma comment(lib, "opencv_calib3d460d.lib")
#pragma comment(lib, "opencv_ccalib460d.lib")
#pragma comment(lib, "opencv_core460d.lib")
#pragma comment(lib, "opencv_features2d460d.lib")
#pragma comment(lib, "opencv_flann460d.lib")
#pragma comment(lib, "opencv_highgui460d.lib")
#pragma comment(lib, "opencv_imgcodecs460d.lib")
#pragma comment(lib, "opencv_imgproc460d.lib")
#pragma comment(lib, "quircd.lib")
#pragma comment(lib, "zlibd.lib")

//#pragma comment(lib, "opencv_ml460d.lib")
//#pragma comment(lib, "opencv_objdetect460d.lib")
//#pragma comment(lib, "opencv_photo460d.lib")
//#pragma comment(lib, "opencv_shape460d.lib")
//#pragma comment(lib, "opencv_stiching460d.lib")
//#pragma comment(lib, "opencv_superres460d.lib")
//#pragma comment(lib, "opencv_video460d.lib")
//#pragma comment(lib, "opencv_videoio460d.lib")
//#pragma comment(lib, "opencv_videostab460d.lib")

#else

#pragma comment(lib, "ade.lib")
#pragma comment(lib, "IlmImf.lib")
#pragma comment(lib, "ippicvmt.lib")
#pragma comment(lib, "ippiw.lib")
#pragma comment(lib, "ittnotify.lib")
#pragma comment(lib, "libjpeg-turbo.lib")
#pragma comment(lib, "libopenjp2.lib")
#pragma comment(lib, "libpng.lib")
#pragma comment(lib, "libprotobuf.lib")
#pragma comment(lib, "libtiff.lib")
#pragma comment(lib, "libwebp.lib")
#pragma comment(lib, "opencv_calib3d460.lib")
#pragma comment(lib, "opencv_ccalib460.lib")
#pragma comment(lib, "opencv_core460.lib")
#pragma comment(lib, "opencv_features2d460.lib")
#pragma comment(lib, "opencv_flann460.lib")
#pragma comment(lib, "opencv_highgui460.lib")
#pragma comment(lib, "opencv_imgcodecs460.lib")
#pragma comment(lib, "opencv_imgproc460.lib")
#pragma comment(lib, "quirc.lib")
#pragma comment(lib, "zlib.lib")

//#pragma comment(lib, "opencv_ml460.lib")
//#pragma comment(lib, "opencv_objdetect460.lib")
//#pragma comment(lib, "opencv_photo460.lib")
//#pragma comment(lib, "opencv_shape460.lib")
//#pragma comment(lib, "opencv_stiching460.lib")
//#pragma comment(lib, "opencv_superres460.lib")
//#pragma comment(lib, "opencv_video460.lib")
//#pragma comment(lib, "opencv_videoio460.lib")
//#pragma comment(lib, "opencv_videostab460.lib")

#endif
//---------------------------------------------------------

#endif // dynamic link, static link