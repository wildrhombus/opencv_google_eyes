
#include "stdio.h"
#include "windows.h"
#include "time.h"
#include <process.h>          // for _beginthread()
#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <sstream>
#include <fstream>
#include <string>

using namespace std;

#include <cxcore.h>
#include <cv.h>
#include "highgui.h"
#include "eyes.h"
#include "codebook.h"
#include "processcapture.h"

#define TRUE 1
#define FALSE 0

#define CHANNELS 2
#define PI 3.14159265

#define MAX_CORNERS 2000
#define MAX_GROUPS 50
#define MAX_CONTOURS 10
#define CVCONTOUR_APPROX_LEVEL 2
#define CVCLOSE_ITR 1

const CvScalar CVX_WHITE = CV_RGB( 0xff, 0xff, 0xff );
const CvScalar CVX_BLACK = CV_RGB( 0x00, 0x00, 0x00 );

