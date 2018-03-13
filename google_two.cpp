
// google_two.cpp
//
//  The main entry point for the app.
//  It initializes everything and starts a thread for the motor control.

#include "stdafx.h"
#include "google_two.h"
#include "signal.h"
#include <windows.h>
#include <stdio.h>
#include "ftd2xx.h"
#include "DynMessage.h"
#include "GoogleUtil.h"

//#define SHOWIMAGE 1
//#define DEBUG 1

#define ANGLEPERPIXEL 60/320
#define CFGFILE "googlecalibration.txt"

double string_to_double( const std::string& s )
{
  std::istringstream i(s);
  double x;
  if( !(i >> x) ) return 0;
  return x;
}

volatile int ctrlc_pressed = 0;
void ctrlc_handler ( int sig )
{
  ctrlc_pressed = 1;
}


int main( int argc, char** argv )
{
  string dir_str;
  string zero_str;
  int index;

  Track *track_eyes = new Track();

/* Use googlecalibration.txt to calibrate the zero position of the eyes */
  ifstream myfile( CFGFILE );
  if( myfile.is_open() )
  {
    while( myfile >> dir_str >> zero_str )
    {
      myfile.ignore( 256, '\n' );
      if( dir_str[0] == 'R' ) index = 0;
      else if( dir_str[0] == 'L' ) index = 1;
      else continue;
      track_eyes->set_zero( index, atoi(zero_str.c_str()) );
    }
    myfile.close();
  }
  else std::cout << "Unable to open calibration file\n";

  Sleep(10000);

  /* Open the thread that controls the motors */
  eyes* o1 = new eyes( track_eyes );
  HANDLE   hth1;
  unsigned  uiThread1ID;

  hth1 = (HANDLE)_beginthreadex( NULL,         // security
                                   0,            // stack size
                                   eyes::ThreadStaticEntryPoint,
                                   o1,           // arg list
                                   CREATE_SUSPENDED,  // so we can later call ResumeThread()
                                   &uiThread1ID );

  if ( hth1 == 0 )
    printf("Failed to create thread 1\n");

  DWORD   dwExitCode;

  GetExitCodeThread( hth1, &dwExitCode );

  ResumeThread( hth1 );

  /* Set the motors to the zero position */

  signal( SIGINT, ctrlc_handler );
  track_eyes->setAction( ZERO );

  /* Setup the Image Capture */
  IplImage *frame;
  long framecount = 0;
  int gcount = 0;
  int ccount = 0;
  char timebuf[26];
  time_t rawtime;
  struct tm timeinfo;

//  CvCapture* capture = cvCreateFileCapture( "Movie.avi" );  // Use a movie instead of the camera
  CvCapture* capture = cvCreateCameraCapture( 0 );
  if( capture == NULL )
  {
    printf( "no capture\n" );
    Sleep(100);
    exit(0);
  }

  frame = cvQueryFrame( capture );
  CvSize sz = cvGetSize( frame );

  ProcessCapture* process_capture = new ProcessCapture( sz, track_eyes );
  CodeBook* code_book = new CodeBook( sz );

  // If show image is set, then the camera output and processing will play on the desktop
  #ifdef SHOWIMAGE
  cvNamedWindow( "google", CV_WINDOW_AUTOSIZE );
  #endif

  // Get the background
  int recalibrate = 1;
  int waiting_at_edge = false;
  framecount = 0;
  track_eyes->setAction( ZERO );
  while( !ctrlc_pressed )
  {
    Sleep(100);
    frame = cvQueryFrame( capture );
    if( !frame ) break;
    framecount++;

    gcount = 0;
    process_capture->init( frame );

    if( framecount > 100 )
    {
      code_book->background_diffs( process_capture->hsv, process_capture->dst );
      ccount = process_capture->find_contours( frame );
    if( ccount > 0 )
    {
        gcount = process_capture->get_groups( frame );
    }
  }

  // Main control
  if( gcount != 0 )
  {
    process_capture->find_eye_position( frame );
    gcount = process_capture->groups.size();
  }
  else if( process_capture->contours.size() != 0 )
  {
    process_capture->find_first();
  }
  else
  {
    if( process_capture->prev_num_contours != 0 )
    {
      // Check for edge cases and wait a little bit at the edge for better visual effect.
      process_capture->endblob = 100;
      waiting_at_edge = false;
      if( ( process_capture->trackx > 570 ) || track_eyes->check_is_stopped() ) {
        track_eyes->setMove( 0, RIGHT, 640 );
        waiting_at_edge = true;
      }
      else if( ( process_capture->trackx < 70 ) || track_eyes->check_is_stopped() ) {
        track_eyes->setMove( 0, LEFT, 0 );
        waiting_at_edge = true;
      }
      else {
        track_eyes->setAction( STOP );
      }
    }
    if( process_capture->endblob > 1 )
    {
      process_capture->trackx = process_capture->prev_trackx;
      process_capture->endblob--;
    }
    if( process_capture->endblob == 1 )
    {
      track_eyes->setAction( ZERO );
      process_capture->trackx = 0;
      process_capture->endblob = 0;
      process_capture->edge_completed = TRUE;
      waiting_at_edge = false;
    }
  }

  if( (framecount <= 100) || (process_capture->groups.size() == 0) )
  {
    code_book->update_codebooks( process_capture->hsv );
  }

  if( process_capture->contours.size() > 0 ) process_capture->get_points( frame );

// Every once in a while refresh the background.  This will essentially erase anything that is not moving.
  if( ((framecount+1) % 100) == 0 )
  {
    code_book->clear_all_stale_entries();
    code_book->update_codebooks( process_capture->hsv );
  }

// Create an initial movement to visually show that the sculpture is on and working.
  if( framecount == 100 )
  {
    track_eyes->setAction( ZERO );
    Sleep(500);
    while( !track_eyes->check_is_stopped() );
    Sleep( 5000 );
    track_eyes->setAction( FARRIGHT );
    Sleep(500);
    while( !track_eyes->check_is_stopped() );
    Sleep( 5000 );
    track_eyes->setAction( FARLEFT);
    Sleep(500);
    while( !track_eyes->check_is_stopped() );
    Sleep( 5000 );
    track_eyes->setAction( ZERO );
    Sleep(500);
    while( !track_eyes->check_is_stopped() );
    recalibrate = !recalibrate;
  }

#ifdef SHOWIMAGE
  cvShowImage( "google", frame );
  uchar bkey = cvWaitKey(7);
  if( bkey == 27 ) break;
#endif
}


// End the threads when program receives signal to stop.
  track_eyes->run_stop();
  track_eyes->DieDieDie();

  WaitForSingleObject( hth1, INFINITE );

  GetExitCodeThread( hth1, &dwExitCode );
  CloseHandle( hth1 );

  delete o1;
  o1 = NULL;

  delete track_eyes;
  track_eyes = NULL;

  cvReleaseCapture( &capture );

  delete process_capture; 
  delete code_book;

  //printf("Primary thread terminating.\n");
  Sleep(100);
  return 0;
}
