#pragma once
#include "eyes.h"
#include "processcapture.h"
#include "codebook.h"

class capture_track
{   
private:
	CvCapture* capture;
	IplImage* frame;
	IplImage* first;   
 	CvSize sz;
	Track* track_eyes;

public:
	capture_track( Track* );
	static unsigned __stdcall ThreadStaticEntryPoint(void* pThis);
    void StartUp();
	virtual ~capture_track(void);
};
