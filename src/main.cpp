#include "libfreenect.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <pthread.h>
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

//#include "cJSON.h"

#include "BlobResult.h"
#include "tracker.h"

// tracker parameters
static const double TMINAREA   = 512;    // minimum area of blob to track
static const double TMAXRADIUS = 24;    // a blob is identified with a blob in the previous frame if it exists within this radius


using namespace cv;
using namespace std;

class Mutex {
public:
	Mutex() {
		pthread_mutex_init( &m_mutex, NULL );
	}
	void lock() {
		pthread_mutex_lock( &m_mutex );
	}
	void unlock() {
		pthread_mutex_unlock( &m_mutex );
	}
private:
	pthread_mutex_t m_mutex;
};

class MyFreenectDevice : public Freenect::FreenectDevice {
  public:
	MyFreenectDevice(freenect_context *_ctx, int _index)
		//: Freenect::FreenectDevice(_ctx, _index), m_buffer_depth(FREENECT_DEPTH_11BIT_SIZE),m_buffer_rgb(FREENECT_VIDEO_RGB_SIZE), m_gamma(2048), m_new_rgb_frame(false), m_new_depth_frame(false),
		: Freenect::FreenectDevice(_ctx, _index), m_buffer_depth(FREENECT_DEPTH_11BIT),m_buffer_rgb(FREENECT_VIDEO_RGB), m_gamma(2048), m_new_rgb_frame(false), m_new_depth_frame(false),
		  depthMat(Size(640,480),CV_16UC1), rgbMat(Size(640,480),CV_8UC3,Scalar(0)), ownMat(Size(640,480),CV_8UC3,Scalar(0))
	{
		for( unsigned int i = 0 ; i < 2048 ; i++) {
			float v = i/2048.0;
			v = std::pow(v, 3)* 6;
			m_gamma[i] = v*6*256;
		}
	}
	// Do not call directly even in child
	void VideoCallback(void* _rgb, uint32_t timestamp) {
		std::cout << "RGB callback" << std::endl;
		m_rgb_mutex.lock();
		uint8_t* rgb = static_cast<uint8_t*>(_rgb);
		rgbMat.data = rgb;
		m_new_rgb_frame = true;
		m_rgb_mutex.unlock();
	};
	// Do not call directly even in child
	void DepthCallback(void* _depth, uint32_t timestamp) {
		//std::cout << "Depth callback" << std::endl;
		m_depth_mutex.lock();
		uint16_t* depth = static_cast<uint16_t*>(_depth);
		depthMat.data = (uchar*) depth;
		m_new_depth_frame = true;
		m_depth_mutex.unlock();
	}

	bool getVideo(Mat& output) {
		m_rgb_mutex.lock();
		if(m_new_rgb_frame) {
			cv::cvtColor(rgbMat, output, CV_RGB2BGR);
			m_new_rgb_frame = false;
			m_rgb_mutex.unlock();
			return true;
		} else {
			m_rgb_mutex.unlock();
			return false;
		}
	}

/*
 Convert depth to U8C1 to avoid copy and convert later.
*/
	bool getDepth8UC1(Mat& output) {
			m_depth_mutex.lock();
			if(m_new_depth_frame) {
				//depthMat.copyTo(output);
    		depthMat.convertTo(output, CV_8UC1, -255.0/2048.0, 255.0);//Invert colors!
				m_new_depth_frame = false;
				m_depth_mutex.unlock();
				return true;
			} else {
				m_depth_mutex.unlock();
				return false;
			}
		}


	bool getDepth(Mat& output) {
			m_depth_mutex.lock();
			if(m_new_depth_frame) {
				depthMat.copyTo(output);
				m_new_depth_frame = false;
				m_depth_mutex.unlock();
				return true;
			} else {
				m_depth_mutex.unlock();
				return false;
			}
		}


  private:
	std::vector<uint8_t> m_buffer_depth;
	std::vector<uint8_t> m_buffer_rgb;
	std::vector<uint16_t> m_gamma;
	Mat depthMat;
	Mat rgbMat;
	Mat ownMat;
	Mutex m_rgb_mutex;
	Mutex m_depth_mutex;
	bool m_new_rgb_frame;
	bool m_new_depth_frame;
};

/*
 * 
 */
void createMask(Mat& src, Mat& oldMask, int nthresh, Mat& mask){

	//Increase dark (near) parts to filter contur errors.
	Mat Kernel(Size(5, 5), CV_8UC1);
	Kernel.setTo(Scalar(1));
	dilate(src, mask, Kernel); 
	dilate(mask, mask, Kernel); 
	dilate(mask, mask, Kernel); 

	/*Mat threshold(src.size(), src.type());
	threshold.setTo(Scalar(nthresh));
	mask = max(mask,threshold);*/
	mask = max(mask+4,nthresh);

	//shift mask to filter small distance errors.
	mask = max(mask,oldMask);
}

//dst will used as tmp val
void filter(Mat& src, Mat& mask, int nthresh, Mat& dst){
	Mat tmp(src.size(), src.type());
	blur(src, tmp, Size(3,3));

	dst = mask < tmp ;
	tmp.copyTo(dst, dst);
	/* this threshold can be integrated in the above mask.
	 * Disadvantage: No adaptive change of this barier possible...
	//threshold(dst, dst, nthresh, 255, THRESH_BINARY);
	threshold(dst, dst, nthresh, 255, THRESH_TOZERO);
	//adaptiveThreshold(dst, dst, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 3, nthresh);
	*/
}


int main(int argc, char **argv) {
	bool die(false);
	string filename("snapshot");
	string suffix(".png");
	int i_snap(0),iter(0);


	int nthresh = 140;
	if( argc > 1){
		nthresh = atoi(argv[1]);
		printf("Threshval: %i\n",nthresh);
	}

	Mat depthMat(Size(640,480),CV_16UC1);
	Mat depthf  (Size(640,480),CV_8UC1);
	Mat filterMat  (Size(640,480),CV_8UC1);
	Mat depthMask  (Size(640,480),CV_8UC1);
	int depthMaskCounter = -30;//use -depthMaskCounter Frame as mask
	Mat filteredMat  (Size(640,480),CV_8UC1);
	Mat rgbMat(Size(640,480),CV_8UC3,Scalar(0));
	Mat ownMat(Size(640,480),CV_8UC3,Scalar(0));

	cTracker tracker(TMINAREA, TMAXRADIUS);
	/*
	CvSize _camSize;
	IplImage* _tmpThresh;
	CBlobResult* _newblobresult;
	CBlobResult* _oldblobresult;
	
	_camSize = cvSize(640,480);
	_tmpThresh = cvCreateImageHeader( _camSize, IPL_DEPTH_8U, 1 );
*/
        //Freenect::Freenect<MyFreenectDevice> freenect;
        //MyFreenectDevice& device = freenect.createDevice(0);
        Freenect::Freenect freenect;
				MyFreenectDevice& device = freenect.createDevice<MyFreenectDevice>(0); 

				// Set vertical Position
				device.setTiltDegrees(0.0);
			
				// Set Led of device
				device.setLed(LED_GREEN);

//	namedWindow("rgb",CV_WINDOW_AUTOSIZE);
	namedWindow("depth",CV_WINDOW_AUTOSIZE);
	namedWindow("filter",CV_WINDOW_AUTOSIZE);
//	device.startVideo();
	device.startDepth();
	//first image = filter mask
	
	/* Das geht hier nicht, da erster Frame komplett schwarz ist */
  //device.getDepth8UC1(depthMask);
	//add const value
	//depthMask -= 5;
	//
	depthMask.setTo(Scalar(255));

    while (!die) {
//   	device.getVideo(rgbMat);
//        cv::imshow("rgb", rgbMat);
//    	device.getDepth(depthMat);
    	device.getDepth8UC1(depthf);
			if( depthMaskCounter < 0){
				depthMaskCounter++;
				if( depthMaskCounter > -25) createMask(depthf,depthMask,nthresh,depthMask);
				/*
				for(int i=0;i<10;i++){
				blur(depthf, depthMask, Size(2,2));
				min(depthMask,depthf,depthMask);
				depthf = depthMask;
				}*/
			}
    	//depthMat.convertTo(depthf, CV_8UC1, 255.0/2048.0);
    	//depthMat.convertTo(depthf, CV_8UC1, 255.0/1048.0);
			//
			//filter
			//subtract(&depthMask, &depthf, &filterMat);
			//filterMat = depthMask - depthf;//überlauf problematisch
			//compare(depthMask,depthf,filterMat,CMP_GT);

			filter(depthf,depthMask,nthresh, filteredMat);

			tracker.trackBlobs(filteredMat, true);
/*
	_tmpThresh->origin = 1;
	_tmpThresh->imageData = (char*) depthf.data;
  _newblobresult = new CBlobResult(_tmpThresh, NULL, 0, false);

		  _newblobresult->Filter( *_newblobresult, B_EXCLUDE, CBlobGetArea(), B_GREATER, val_blob_maxsize.internal_value );
			  _newblobresult->Filter( *_newblobresult, B_EXCLUDE, CBlobGetArea(), B_LESS, val_blob_minsize.internal_value );
*/

        cv::imshow("depth",depthMask);
        cv::imshow("filter",filteredMat);
        //cv::imshow("filter",filterMat);
		char k = cvWaitKey(5);
		if( k == 27 ){
//		    cvDestroyWindow("rgb");
		    //cvDestroyWindow("depth");
			break;
		}
		if( k == 8 ) {
			std::ostringstream file;
			file << filename << i_snap << suffix;
			cv::imwrite(file.str(),rgbMat);
			i_snap++;
		}
		if(iter >= 20000) break;
		iter++;
    }

		    cvDestroyWindow("depth");
		    cvDestroyWindow("filter");
//   	device.stopVideo();
	cvDestroyWindow("depth");
	cvDestroyWindow("filter");
	device.stopDepth();


	return 0;
}

