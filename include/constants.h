#ifndef CONSTANTS_H
#define CONSTANTS_H

/* Resoluton of kinect images */
//#define KRES_X = 640;
//#define KRES_Y = 480;
static const int KRES_X = 640;
static const int KRES_Y = 480;

/* Number of Frames to generate depth mask. Should be at least 5 */
static const int NMASKFRAMES = 30;

// tracker parameters
static const double TMINAREA   = 512;    // minimum area of blob to track
static const double TMAXRADIUS = 24;    // a blob is identified with a blob in the previous frame if it exists within this radius
//
//

/* store which changes need attention */
enum Changes {NO=0,MASK=1,MOTOR=2,CONFIG=4,MARGIN=8,AREAS=16, ALL=1023};

inline double min(double a,double b){return a<b?a:b;};
inline double max(double a,double b){return a>b?a:b;};

#endif
