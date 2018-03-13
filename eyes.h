#pragma once
#include <string>             // for STL string class
#include <windows.h>          // for HANDLE
#include <process.h>          // for _beginthread()
#include <cxcore.h>
#include <cv.h>
#include "highgui.h"
#include "time.h"
#include <vector>
#include "ftd2xx.h"
#include "DynMessage.h"
#include "GoogleUtil.h"

using namespace std;

#define LEFT 1
#define RIGHT 0
#define NO_DIR -1

#define TRUE 1
#define FALSE 0

#define PI 3.14159265

#define NOACTION -1
#define STOP 0
#define MOVE 1
#define ZERO 2
#define FARLEFT 3
#define FARRIGHT 4

#define NORMAL 0
#define OVERSHOOT 1
#define CHANGEDIR 2

void error ( int result );
void setAcceleration( int servo, double accel );
void setVelocity( int servo, double state );
void setPosition( int servo, int state );

typedef struct motor_msgs {
	vector<unsigned __int8> setPositionRight;
	vector<unsigned __int8> setPositionLeft;
	vector<unsigned __int8> setPositionZero;
	vector<unsigned __int8> setEnableON;
	vector<unsigned __int8> setEnableOFF;
	vector<unsigned __int8> getSpeed;
	vector<unsigned __int8> getPosition;
	vector<unsigned __int8> getEnable;
	vector<unsigned __int8> getMoving;
	vector<unsigned __int8> setStatus;
	vector<unsigned __int8> setReturnDelay;
	vector<unsigned __int8> setCWLimit;
	vector<unsigned __int8> setCCWLimit;
	vector<unsigned __int8> setCW_CM_msg;
	vector<unsigned __int8> setCCW_CM_msg;
	vector<unsigned __int8> setCW_Slope_msg;
	vector<unsigned __int8> setCCW_Slope_msg;
	vector<unsigned __int8> getCW_CM_msg;
	vector<unsigned __int8> getCCW_CM_msg;
	vector<unsigned __int8> getCW_Slope_msg;
	vector<unsigned __int8> getCCW_Slope_msg;
	vector<unsigned __int8> getPunch_msg;
} motorMsgs;

typedef struct eye_info {
	unsigned __int8 motor_id;
	int dir;
	double x;
	double speed;
	double smooth_speed;
	unsigned __int16 minVel;
	unsigned __int16 maxVel;
	unsigned __int16 zero_pos;
	unsigned __int16 right_pos;
	unsigned __int16 left_pos;
	double cur_x;
	double future_x;
} eyeInfo;

class Track
{
private:
  FT_HANDLE ftHandle;
  double smooth_dst;
  CRITICAL_SECTION m_CriticalSection;
  HANDLE  m_hEvent;
  BOOL  m_bContinueProcessing;      // used to control thread lifetime

  int overshoot_count[2];
  int trackAction;
  double dist;
  int new_dir; 

  int read_errors;
  int write_errors;
  
  //double find_eye_loc ( int, double, double, int );
  void stop_eye( int );
 // void start_eyes( int );
  void change_direction( int, int );
 // void check_move_done();
 // void find_speed_div();
  int motorWrite( vector<unsigned __int8> ); 
  int motorRead();
  int Track::setSpeed( unsigned __int16, int );
  int Track::getSpeed( int );
  int Track::setRightPosition( int );
  int Track::setLeftPosition( int );
  int Track::setZeroPosition( int );
  int Track::getPosition( int );
  int Track::setEnableON( int );
  int Track::setEnableOFF( int );
  int Track::getEnable( int );
  int Track::getIsMoving( int );
  void Track::readStatus();
  void Track::motor_ping();

public:
  eyeInfo eye[2];
  motorMsgs msgs[2];

  Track();
  virtual ~Track();
  void setAction( int );
  void setMove( double, int, double );
  void processActions();
  void DieDieDie();
  int check_is_stopped();
  void run_stop();
  int run_move();
  void run_zero();
  void run_farleft();
  void run_farright();
  void set_zero( int index, unsigned __int16 zero );
};

class eyes
{
private:
	Track* track;

public:
	eyes( Track* );
	static unsigned __stdcall ThreadStaticEntryPoint(void * pThis);
    void StartUp();
	void DieDieDie();
	virtual ~eyes(void);
};



