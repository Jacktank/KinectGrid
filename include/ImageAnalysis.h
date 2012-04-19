#ifndef IMAGEANALYSIS_H
#define IMAGEANALYSIS_H

#include "constants.h"
#include "MyFreenectDevice.h"
#include "SettingKinect.h"

class ImageAnalysis{
public:
	ImageAnalysis(MyFreenectDevice* device, SettingKinect* pSettingKinect);
	~ImageAnalysis();
	void analyse();
	void resetMask(SettingKinect* pSettingKinect, int changes);
public:
	Mat m_depthf  ;
	Mat m_filterMat;
	Mat m_depthMask;
	Mat m_depthMaskWithoutThresh;
	Mat m_filteredMat;
private:
	MyFreenectDevice* m_pdevice;
	SettingKinect* m_pSettingKinect;
	int m_depthMaskCounter;//use -depthMaskCounter Frames for mask generation
};

#endif
