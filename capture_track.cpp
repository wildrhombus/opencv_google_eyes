#include "StdAfx.h"
#include "capture_track.h"

#define DEBUG 1

capture_track::capture_track(Track* t)
{
  track_eyes = t;
}

unsigned __stdcall capture_track::ThreadStaticEntryPoint(void * pThis)
{
  capture_track * pthctrack = (capture_track*)pThis;   // the tricky cast
  pthctrack->StartUp();           // now call the true entry-point-function

  return 1;
}

void capture_track::StartUp()
{
//  CvCapture* capture = cvCreateFileCapture( "Movie.avi" );
  CvCapture* capture = cvCreateCameraCapture( 0 );
  if( capture == NULL )
  {
    printf( "no capture\n" );
    return;
  }

  first = cvQueryFrame( capture );
    sz = cvGetSize( first );

  ProcessCapture* process_capture = new ProcessCapture( sz, track_eyes );
  CodeBook* code_book = new CodeBook( sz );

  //cvNamedWindow( "google", CV_WINDOW_AUTOSIZE );

  int recalibrate = 1;
  if( !recalibrate ) code_book->read_from_disk();

  long framecount = 0;
  int gcount = 0;
  int ccount = 0;

  clock_t start_t, end_t;
  int processgroupcount = 0;
  double cycle_average = 100.0;
  int isContour = FALSE;
    framecount = 0;
  track_eyes->setAction( ZERO );
  while(1)
    {
    start_t = clock();
      frame = cvQueryFrame( capture );
      if( !frame ) break;
    framecount++;
     
    gcount = 0;
    process_capture->init( frame );
    
    isContour = FALSE;

    if( (recalibrate && framecount <= 100) || process_capture->groups.size() == 0 ) code_book->update_codebooks( process_capture->hsv );
    else code_book->update_codebooks_mask( process_capture->hsv, process_capture->cb_mask );

    end_t = clock();
  }
  track_eyes->setAction( STOP );
}


capture_track::~capture_track(void)
{
  cvReleaseCapture( &capture );
  cvReleaseImage( &frame );
  cvReleaseImage( &first );
}
