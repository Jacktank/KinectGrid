#ifndef IMAGEANALYSIS_H
#define IMAGEANALYSIS_H

#include "MyFreenectDevice.h"

class ImageAnalysis{
public:
	ImageAnalysis(MyFreenectDevice* device);
	~ImageAnalysis();
	void analyse();
private:
	Mat m_depthMat;
public:
	Mat m_depthf  ;
	Mat m_filterMat;
	Mat m_depthMask;
	Mat m_filteredMat;
private:
	MyFreenectDevice* m_pdevice;
	int m_depthMaskCounter;//use -depthMaskCounter Frames for mask generation


};

#endif
