// Thread for motor control

#include "StdAfx.h"
#include <windows.h>
#include <iostream>
#include <cmath>
#include <ctime>
#include "eyes.h"

//#define MOTORS 1
//#define DEBUG 1
#define SERVO1 1
#define SERVO2 2

#define EDIFF 257
#define E_RANGE (EDIFF*2.0)

#define GRANGE 640.0
#define GLEFT 0.0
#define GZERO (GRANGE/2.0)
#define GRIGHT GRANGE

#define G_2_E E_RANGE/GRANGE
#define E_2_G GRANGE/E_RANGE

#define INTERVAL 0.7
#define MAXSPEED 0x3FF
#define MINSPEED 5
#define SLOPE 0x80
#define CM 0x01

CRITICAL_SECTION p_CriticalSection;

inline unsigned __int16 myRound(float x) { return (floor(x + 0.5)); }

unsigned __int8 motorIDS[] = { 0x01, 0x02 };

int rebootOnError() {
  HANDLE hToken;
  TOKEN_PRIVILEGES tkp;

  // Get a token for this process
  if (!OpenProcessToken(GetCurrentProcess(),
    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
    printf("OpenProcesstoken failed!\n");
    return 0;
  }

  // Get the LUID for the shutdown privilege.
  LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

  tkp.PrivilegeCount = 1;
  tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

  if (GetLastError() != ERROR_SUCCESS) {
    printf("AdjustTokenPrivileges\n");
    return 0;
  }

  BOOL b = ExitWindowsEx( EWX_REBOOT|EWX_FORCE, SHTDN_REASON_MINOR_MAINTENANCE | SHTDN_REASON_FLAG_PLANNED);

  DWORD dwerr = 0;
  dwerr = GetLastError();
  //printf("rebooting\n");
  return 0;
}


Track::Track()
{
  InitializeCriticalSection(&m_CriticalSection);
  InitializeCriticalSection(&p_CriticalSection);

  read_errors = 0;
  write_errors = 0;

  int i;
  for( i = 0; i < 2; i++ )
  {
    eye[i].x = -9999;
    eye[i].dir = LEFT;
    eye[i].speed = 0;
    eye[i].smooth_speed = 30;
    eye[i].zero_pos = kGooglePosition_Centre;
    eye[i].left_pos = kGooglePosition_Centre + EDIFF;
    eye[i].right_pos = kGooglePosition_Centre - EDIFF;
    overshoot_count[i] = 0;
  }

  smooth_dst = 20;

  m_bContinueProcessing = TRUE;   // will be changed upon a call to DieDieDie()
  m_hEvent = CreateEvent( NULL, FALSE, FALSE, NULL ); 
  if (m_hEvent == NULL) { 
    std::cout << "CreateEvent() failed in MessageBuffer ctor." << endl;
    write_errors++;
  }

  //open the device for connections
  FT_STATUS ftStatus;
  DWORD numDevs;

  ftStatus = FT_CreateDeviceInfoList(&numDevs);
  if( ftStatus == FT_OK) {
    //std::cout << "Number of devices is 2" << endl;
  } else {
    std::cout << "Create Device Info List failed with error " << ftStatus << endl;
    write_errors++;
  }

  //Open channel to ftdi uart, used by both motors
  ftStatus = FT_Open(0, &ftHandle);
  if( ftStatus != FT_OK) {
//  std::cout << "Opening failed! with error " << ftStatus << endl;
	ftStatus = FT_Open(0, &ftHandle);
    if( ftStatus != FT_OK) {
	 // std::cout << "Opening failed! with error " << ftStatus << endl;
	  write_errors++;
	  return;
    }
  }

  ftStatus = FT_SetBaudRate(ftHandle, 57600);
  if( ftStatus != FT_OK ) {
    //std::cout << "set baud rate failed with error " << ftStatus << endl;
  write_errors++;
  return;
  }

  ftStatus = FT_SetDataCharacteristics(ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
  if( ftStatus != FT_OK ) {
    //std::cout << "Set data characteristics failed" << endl;
    write_errors++;
    return;
  }

  for( i = 0; i < 2; i++ ) {
    unsigned __int8 id = motorIDS[i];
    eye[i].motor_id = id;

    unsigned __int16 posLeft = kGooglePosition_Centre - EDIFF;
    unsigned __int16 posRight = kGooglePosition_Centre + EDIFF;

    msgs[i].setPositionRight = DynMessage::SetPosition(posRight, id);
    msgs[i].setPositionLeft  = DynMessage::SetPosition(posLeft, id);
    msgs[i].setPositionZero = DynMessage::SetPosition(kGooglePosition_Centre, id);
    msgs[i].setEnableON = DynMessage::SetEnable(0x01, id);
    msgs[i].setEnableOFF = DynMessage::SetEnable(0x00, id);
    msgs[i].getSpeed = DynMessage::GetSpeed(id);
    msgs[i].getPosition = DynMessage::GetPosition(id);
    msgs[i].getEnable = DynMessage::GetEnable(id);
    msgs[i].getMoving = DynMessage::GetMoving(id);
    msgs[i].setStatus = DynMessage::SetStatus(0x01, id);
    msgs[i].setReturnDelay = DynMessage::SetReturnDelay(0x10, id);
    msgs[i].setCWLimit = DynMessage::SetClockwiseLimit(posRight, id);
    msgs[i].setCCWLimit = DynMessage::SetCounterClockwiseLimit(posLeft, id);
    msgs[i].setCW_CM_msg = DynMessage::SetCW_CM(CM, id);
    msgs[i].setCCW_CM_msg = DynMessage::SetCCW_CM(CM, id);
    msgs[i].setCW_Slope_msg = DynMessage::SetCW_Slope(SLOPE, id);
    msgs[i].setCCW_Slope_msg = DynMessage::SetCCW_Slope(SLOPE, id);
    msgs[i].getCW_CM_msg = DynMessage::GetCW_CM(id);
    msgs[i].getCCW_CM_msg = DynMessage::GetCCW_CM(id);
    msgs[i].getCW_Slope_msg = DynMessage::GetCW_Slope(id);
    msgs[i].getCCW_Slope_msg = DynMessage::GetCCW_Slope(id);
    msgs[i].getPunch_msg = DynMessage::GetPunch(id);

/*
    if( !motorWrite( msgs[i].setStatus ) ) {
      //std::cout << "status level write failed " << endl;
    }

    if( !motorWrite( msgs[i].setReturnDelay ) ) {
      //std::cout << "return delay write failed with error  " << ftStatus <<endl;
    }
  
    if( !motorWrite( msgs[i].setCWLimit ) ) {
      //std::cout << "write cw limit failed" << endl;
    } 

    if( !motorWrite( msgs[i].setCCWLimit ) ) {
      //std::cout << "write ccw limit failed\n" << endl;
    }

    if( !motorWrite( msgs[i].setCW_CM_msg ) ) {
      //std::cout << "set cw cm failed" << endl;
    }

    if( !motorWrite( msgs[i].setCCW_CM_msg ) ) {
      //std::cout << "set ccw cm msg failed" << endl;
    }

    if( !motorWrite( msgs[i].setCW_Slope_msg ) ) {
      //std::cout << "set cw slope msg failed" << endl;
    }

    if( !motorWrite( msgs[i].setCCW_Slope_msg ) ) {
      //std::cout << "set ccw slope msg failed" << endl;
    }
    */

    setSpeed( eye[i].maxVel/8, i );

    Sleep ( 100 );
  }
}

Track::~Track()
{
  int i;

  for( i = 0; i < 2; i++ )
  {
    setSpeed(1, i);
  }

  FT_Close(ftHandle);

  DeleteCriticalSection(&m_CriticalSection);
  CloseHandle( m_hEvent );
}

void Track::setAction( int Action )
{
  EnterCriticalSection( &m_CriticalSection );
  trackAction = Action;

  if ( ! SetEvent( m_hEvent ) ) std::cout <<  "SetEvent() failed in SetMessage()" << endl;
  LeaveCriticalSection( &m_CriticalSection );
}

void Track::setMove( double g_dist,  int g_dir, double g_x )
{
 // std::cout << g_dist << " g_dist " <<  g_dir << " g_dir " << " g_x " << g_x;
  double gdist_steps;
  EnterCriticalSection( &m_CriticalSection );
  trackAction = MOVE;
  gdist_steps = G_2_E*g_dist;
  smooth_dst = cvRound(0.9*gdist_steps + 0.1*smooth_dst); 
  dist = smooth_dst;
  new_dir = g_dir;
  for( int i = 0; i < 2; i++ )
  {
    eye[i].cur_x = G_2_E*(GRANGE - g_x) + eye[i].right_pos;
  }

  LeaveCriticalSection( &m_CriticalSection );
}

void Track::processActions()
{
  int newAction;

  while ( m_bContinueProcessing )   // state variable used to control thread lifetime
  {
    DWORD   dwWaitResult = WaitForSingleObject( m_hEvent, 5 );
    if ( ( dwWaitResult == WAIT_TIMEOUT )   // WAIT_TIMEOUT = 258
          && ( m_bContinueProcessing == false ) )
    {
      break;    // we were told to die
    }
    else if ( dwWaitResult == WAIT_ABANDONED ) // WAIT_ABANDONED = 80
    {
      //printf( "WaitForSingleObject failed in Track::ProcessActions.\n");
      return;
    }

	EnterCriticalSection( &m_CriticalSection );
    newAction = trackAction;
	trackAction = NOACTION;
    LeaveCriticalSection( &m_CriticalSection );

    if( read_errors > 100 ) {
//    printf("more than 100 read errors\n");
      rebootOnError();
    }

    if( write_errors > 100 )  {
//    printf("more than 10 write errors\n");
      rebootOnError();
    }

    if( newAction == STOP ) run_stop();
    if( newAction == ZERO ) run_zero();
    if( newAction == FARLEFT ) run_farleft();
    if( newAction == FARRIGHT ) run_farright();
    if( newAction == MOVE ) run_move();
  }
  run_stop();
}

void  Track::DieDieDie( void )
{
  //printf("Track diediedie\n");
  Sleep( 10 );
  m_bContinueProcessing = false;   // ProcessMessages() watches for this in a loop
  run_stop();
}

int Track::check_is_stopped() {
  int isStopped = 0;
  if( !m_bContinueProcessing ) return TRUE;

  for( int i = 0; i < 2; i++ )
  {
    int isMoving = getIsMoving(i);
	if( isMoving == 0 ) isStopped++; 
  }
  return (isStopped == 2); 
}

void Track::run_farright()
{
  int i;
  for( i = 0; i < 2; i++ )
  {
	eye[i].dir = RIGHT;
	eye[i].speed = 0x200 * 1.0;
	setSpeed( 0x200, i );
  }

  for( i = 0; i < 2; i++ )
  {
    setRightPosition( i );
  }
}

void Track::run_farleft()
{
  int i;
  for( i = 0; i < 2; i++ )
  {
    eye[i].dir = LEFT;
	eye[i].speed = 0x200 * 1.0;
	setSpeed( 0x200, i );
  }

  for( i = 0; i < 2; i++ )
  {
	setLeftPosition(i);
  }
}

void Track::run_zero()
{
  int i;

  for( i = 0; i < 2; i++ )
  {
	eye[i].speed = 0x80 * 1.0;
 	setSpeed( 0x80, i );
  }
  for( i = 0; i < 2; i++ )
  {
    setZeroPosition( i );
  }
}

int Track::run_move()
{
  double dst;
  int g_dir[2], prev_dir[2], next_dir[2];
  double g_x[2];
  double future[2];
  int i;

  EnterCriticalSection( &m_CriticalSection );
  dst = dist;
  for( i = 0; i < 2; i++ )
  {
    g_x[i] = eye[i].cur_x;
	g_dir[i] = new_dir;
  }
  LeaveCriticalSection( &m_CriticalSection );

  for( i = 0; i < 2; i++ )
  {
    eye[i].x = getPosition( i )*1.0;
  }

  int edge[2];
  for( i = 0; i < 2; i++ )
  {
    edge[i] = false;
    if( ( g_x[i] <= ( eye[i].right_pos + 40 ) && g_dir[i] == RIGHT )
      ||( g_x[i] >= ( eye[i].left_pos - 40 ) && g_dir[i] == LEFT ) )
    {
#ifdef DEBUG
    std::cout << i << " cd a is edge " << g_x[i] << endl;
#endif
	  edge[i] = true;
	  unsigned __int16 edge_pos = 0;
	  if( g_dir[i] == RIGHT ) {
		  edge_pos = eye[i].right_pos;
	  }
	  if( g_dir[i] == LEFT ) {
		  edge_pos = eye[i].left_pos;
	  }
	  if( abs(edge_pos - myRound(eye[i].x) ) < 200 ) {
        change_direction( g_dir[i], i );
      eye[i].dir = g_dir[i];
    }
    setSpeed( 300, i );
  }
  }

  if( edge[0] && edge[1] ) {
#ifdef DEBUG
  std::cout << "both edge return" << endl;
#endif
  return 0;
  }

  if( ( g_dir[0] == LEFT && ( eye[0].x >= eye[0].left_pos ) && ( eye[1].x >= eye[1].left_pos ) ) ||
      ( g_dir[0] == RIGHT && ( eye[0].x <= eye[0].right_pos ) && ( eye[1].x <= eye[1].right_pos ) ) )
  {
#ifdef DEBUG
  std::cout << " over edge " << endl;
#endif
    return 0;
  }

  for( i = 0; i < 2; i++ )
  { 
 //   if( getIsMoving(i) ) {
	  prev_dir[i] = eye[i].dir;
 //   } else {
//    prev_dir[i] = NO_DIR;
//    }
  }

  for( i = 0; i < 2; i++ )
  {
    if( g_dir[i] == LEFT ) future[i] = g_x[i] + dst;
    else future[i] = g_x[i] - dst;
    if( future[i] < eye[i].right_pos ) future[i] = eye[i].right_pos;
    if( future[i] > eye[i].left_pos ) future[i] = eye[i].left_pos;

    if( eye[i].x > future[i] ) next_dir[i] = RIGHT;
  else next_dir[i] = LEFT;
  }

  int overshoot[2];
  for( i = 0; i < 2; i++ ) {
    overshoot[i] = false;
  if( !edge[i] && (next_dir[i] != prev_dir[i]) && (prev_dir[i] == g_dir[i]) )
    {
      if( overshoot_count[i]++ <= 1 ) overshoot[i] = true;
    else overshoot_count[i] = 0;
    }
  }

  for( i = 0; i < 2; i++ )
  {
    if( overshoot[i] ) {
    eye[i].dir = g_dir[i];
    setSpeed( 5, i );
#ifdef DEBUG
    std::cout << i << " cd b " << endl;
#endif
    change_direction( g_dir[i], i );
  }
  }
  if( overshoot[0] && overshoot[1] ) {
#ifdef DEBUG
    std::cout << "is both overshoot" << endl;
#endif
    return 0;
  }

  int change[2];
  for( i = 0; i < 2; i++ ) {
    change[i] = true;
    if( overshoot[i] || edge[i] ) change[i] = false;
  }

  for( i = 0; i < 2; i++ )
  {
    if( change[i] ) {
#ifdef DEBUG
      std::cout << i << " future " << future[i] << " now " << eye[i].x << endl;
#endif

    eye[i].speed = fabs( future[i] - eye[i].x )/ INTERVAL;
#ifdef DEBUG
    std::cout << i << " before smooth speed " << eye[i].speed << endl;
#endif
    //***KEY FOR ADJUSTING REACTION - play with percentage.
      eye[i].speed = cvRound(0.91*eye[i].speed + 0.09*eye[i].smooth_speed); 

    if( eye[i].speed > 200 ) eye[i].speed = 200;
    if( eye[i].speed < 5 ) eye[i].speed = 5;
#ifdef DEBUG
    std::cout << i << " speed " << eye[i].speed << endl;
#endif
   }
  }

  for( i = 0; i < 2; i++ )
  {
    if( change[i] ) {
      setSpeed( round(eye[i].speed), i );
	}
  }
  
  for( i = 0; i < 2; i++ )
  {
    if( change[i] ) {
	  overshoot_count[i] = 0;
	  if( next_dir[i] != prev_dir[i] ) {
		  change_direction ( next_dir[i], i );
#ifdef DEBUG
		  std::cout << i << " cd c " << next_dir[i] << " " << new_dir << endl;
#endif
	  }
	  eye[i].dir = next_dir[i];
	}
  }

  return 0;
}

void Track::run_stop()
{
  int i;
#ifdef DEBUG
  std::cout << "run stop" << endl;
#endif

  for( i = 0; i < 2; i++ )
  {
  setSpeed( 1, i );
  eye[i].speed = 1;
  }

  for(i = 0; i < 2; i++ )
  {
    eye[i].x = getPosition( i ) * 1.0;
    eye[i].dir = NO_DIR;
  }
}

void Track::change_direction( int new_direction, int eye_num )
{
  eye[eye_num].dir = new_direction;

  if( new_direction == LEFT ) {
    setLeftPosition( eye_num );
  }

  if( new_direction == RIGHT ) {
  setRightPosition( eye_num );
  }
}

void Track::set_zero( int index, unsigned __int16 zero ) {
  if( zero == 0 ) std::cout << "zero not read correctly from googlecalibration.txt file" << endl;
  if( zero <= EDIFF ) std::cout << "zero value is too small, it must be greater than " << EDIFF << endl;
 
  unsigned __int16 posLeft = zero + EDIFF;
  unsigned __int16 posRight = zero - EDIFF;

  msgs[index].setCWLimit = DynMessage::SetClockwiseLimit(posRight, eye[index].motor_id);
  msgs[index].setCCWLimit = DynMessage::SetCounterClockwiseLimit(posLeft, eye[index].motor_id);
  msgs[index].setPositionRight = DynMessage::SetPosition(posRight, eye[index].motor_id);
  msgs[index].setPositionLeft  = DynMessage::SetPosition(posLeft, eye[index].motor_id);
  msgs[index].setPositionZero = DynMessage::SetPosition(zero, eye[index].motor_id);

  eye[index].zero_pos = zero;
  eye[index].right_pos = posRight;
  eye[index].left_pos = posLeft;


  if( !motorWrite( msgs[index].setCWLimit ) ) {
    std::cout << "write cw limit failed" << endl;
  } 

  if( !motorWrite( msgs[index].setCCWLimit ) ) {
    std::cout << "write ccw limit failed\n" << endl;
  }
}

int Track::setSpeed(unsigned __int16 speed, int eye_num) {
  //1 is 0 speed for Dymixel motors
  if( speed < 1 ) speed = 1;

  vector<unsigned __int8> setSpeed = DynMessage::SetSpeed(speed, eye[eye_num].motor_id);
  if( !motorWrite( setSpeed ) ) {
    //std::cout << "set speed " << speed << " write failed" << endl;
  return false;
  }
  return true;
}

int Track::getSpeed(int eye_num) {
  int speed;

  if( !motorWrite( msgs[eye_num].getSpeed ) ) {
    //std::cout << "getSpeed write request failed" << endl;
    return -1;
  }

  speed = motorRead();
  if( speed == -1 ) {
    //std::cout << "read speed failed" << endl;
    return -1;
  }
  return speed;
}

int Track::setRightPosition(int eye_num) {
  if( !motorWrite( msgs[eye_num].setPositionRight ) ) {
    //std::cout << "set position right failed" << endl;
    return false;
  }
  return true;
}

int Track::setLeftPosition(int eye_num) {
  if( !motorWrite( msgs[eye_num].setPositionLeft ) ) {
    //std::cout << "set position left failed" << endl;
    return false;
  }
  return true;
}

int Track::setZeroPosition(int eye_num) {
  if( !motorWrite( msgs[eye_num].setPositionZero ) ) {
    //std::cout << "set position zero failed" << endl;
    return false;
  }
  return true;
}

int Track::getPosition( int eye_num ) {
  int position;

  if( !motorWrite( msgs[eye_num].getPosition ) ) {
    //std::cout << "getPosition write request failed" << endl;
    return -1;
  }

  position = motorRead();
  if( position == -1 ) {
  //  time_t t = std::time(0);
  //  struct tm * now = std::localtime(&t);
  //  std::cout <<(now->tm_hour) << ":" << (now->tm_min) << ":" << (now->tm_sec) << " - ";
  //  std::cout << "read position failed" << endl;
    return -1;
  }
  return position;
}

int Track::setEnableON(int eye_num) {
  if( !motorWrite( msgs[eye_num].setEnableON ) ) {
    //std::cout << "set position right failed" << endl;
    return false;
  }
  return true;
}

int Track::setEnableOFF(int eye_num) {
  if( !motorWrite( msgs[eye_num].setEnableOFF ) ) {
    //printf("set position right failed\n");
    return false;
  }
  return true;
}

int Track::getEnable( int eye_num ) {
  int enabled;

  if( !motorWrite( msgs[eye_num].getEnable ) ) {
    //std::cout << "getEnable write request failed" << endl;
    return -1;
  }

  enabled = motorRead();
  if( enabled == -1 ) {
    //std::cout << "read enable failed" << endl;
    return -1;
  }
  return enabled;
}

int Track::getIsMoving( int eye_num ) {
  int isMoving;

  if( !motorWrite( msgs[eye_num].getMoving ) ) {
    //std::cout << "getMoving write request failed" << endl;
    return -1;
  }

  isMoving = motorRead();
  if( isMoving == -1 ) {
    //std::cout << "read moving failed" << endl;
    return -1;
  }
  return isMoving;
}

void Track::readStatus() {
  FT_STATUS ftStatus;
  DWORD EventDWord;
  DWORD TxBytes;
  DWORD RxBytes;
  DWORD BytesReceived;
  char RxBuffer[256];

  FT_GetStatus(ftHandle, &RxBytes, &TxBytes, &EventDWord);
  if( RxBytes > 0 ) {
    ftStatus = FT_Read( ftHandle, RxBuffer, RxBytes, &BytesReceived );
    if( ftStatus != FT_OK ) {
      std::cout << "FT_Read failed with error " << endl;
      read_errors++;
    } else {
      std::cout << " Status: " << endl;
      for( unsigned int j = 0; j < BytesReceived; j++ ) {
        printf("0x%x ", RxBuffer[j]);
      }
      printf("\n");

    }
  } else {
    read_errors++;
    //std::cout << " FT_GetStatus returned 0 bytes" << endl;
  }
}

void Track::motor_ping() {
  FT_STATUS ftStatus;

  unsigned char ping[6] = { 0xFF, 0xFF, 0x01, 0x02, 0x01, 0xFB };
  DWORD BytesWritten;

  //std::cout << "ping";
    ftStatus = FT_Write(ftHandle, ping, 6, &BytesWritten);
  if( ftStatus != FT_OK ) {
    //std::cout << "can't ping" << endl;
    write_errors++;
  }

  Sleep( 20 );

  readStatus();
}

int Track::motorWrite( vector<unsigned __int8> msg ) 
{
  FT_STATUS ftStatus;
  DWORD BytesWritten;

    ftStatus = FT_Write(ftHandle, &msg[0], msg.size(), &BytesWritten);
  if( ftStatus != FT_OK ) {
    //std::cout << "write status " << ftStatus << endl;
    write_errors++;
    return false;
  }
  if( write_errors > 0 ) {
    write_errors--;
    //std::cout << "write self correcting" << endl;
  }
  return true;
}

int Track::motorRead() {
  FT_STATUS ftStatus;
  DWORD EventDWord;
  DWORD TxBytes;
  DWORD RxBytes;
  DWORD BytesReceived;
  unsigned char RxBuffer[256];
  //time_t t = std::time(0);
  //struct tm * now = std::localtime(&t);
  //std::cout <<(now->tm_hour) << ":" << (now->tm_min) << ":" << (now->tm_sec) << " - ";

  int statuswait = 0;
  RxBytes = 0;
  while( RxBytes == 0 ) {
    Sleep(20);
    FT_GetStatus(ftHandle, &RxBytes, &TxBytes, &EventDWord);
    statuswait++;
    if( statuswait > 50 ) {
      //std::cout << "no status bytes" << endl;
      read_errors++;
      return -1;
    }
  }

  ftStatus = FT_Read( ftHandle, RxBuffer, RxBytes, &BytesReceived );
  if( ftStatus == FT_OK ) {
    if(BytesReceived < 6) {
      read_errors++;
      //std::cout << "less than 6 bytes" << endl;
      return -1;
    }

    // test header
    if(RxBuffer[0] != kDYN_HeaderByte ||
      RxBuffer[1] != kDYN_HeaderByte)
    {
//      std::cout <<(now->tm_hour) << ":" << (now->tm_min) << ":" << (now->tm_sec) << " - ";
      //std::cout << "bad header" << endl;
      read_errors++;
      return -1;
    }
  
  // test checksum
    unsigned __int8 expectedChecksum = 0;

    for(unsigned int i = 2; i < BytesReceived-1; i++)
    {
      expectedChecksum += RxBuffer[i];
    }
    expectedChecksum = ~expectedChecksum;
  
    if(expectedChecksum != RxBuffer[BytesReceived-1] )
    {
//      std::cout <<(now->tm_hour) << ":" << (now->tm_min) << ":" << (now->tm_sec) << " - ";
      //std::cout << "bad checksum" << endl;
      read_errors++;
      return -1;
    }

    if(RxBuffer[4] != 0 ) {
//      std::cout <<(now->tm_hour) << ":" << (now->tm_min) << ":" << (now->tm_sec) << " - ";
      unsigned __int8 errorByte = RxBuffer[4];
      if(errorByte & (1 << 0)) {
        std::cout << "Voltage error" << endl;
      }
      if(errorByte & (1 << 1)) {
        std::cout << "Angle limit error" << endl;
      }
      if(errorByte & (1 << 2)) {
        std::cout << "Overheat error" << endl;
      }
      if(errorByte & (1 << 3)) {
        std::cout << "Range error" << endl;
      }
      if(errorByte & (1 << 4)) {
        std::cout << "Checksum error" << endl;
      }
      if(errorByte & (1 << 5)) {
        std::cout << "Overload error" << endl;
      }
      if(errorByte & (1 << 6)) {
        std::cout << "Instruction error" << endl;
      }
      read_errors++;
      return -1;
    }

    unsigned __int16 param = RxBuffer[5];
  
    // if this is a two byte param, add the second byte
    if(BytesReceived > 7)
    {
      param += ((unsigned __int16)RxBuffer[6]) << 8;
    }

    if( read_errors > 0 ) {
      read_errors--;
      //std::cout << "Read self correcting" << endl;
    }
    return param;
  } else {
    read_errors++;
  //  std::cout << "FT_read failed" << endl;
    return -1;
  }
}


eyes::eyes(Track* t)
{
  track = t;
}

unsigned __stdcall eyes::ThreadStaticEntryPoint(void * pThis)
{
  eyes * ptheyes = (eyes*)pThis;   // the tricky cast
  ptheyes->StartUp();           // now call the true entry-point-function

  return 1;
}

void eyes::StartUp()
{
  track->processActions();
}

void eyes::DieDieDie( void )
{
  //std::cout << "DieDieDie" << endl;
  track->DieDieDie();  
}

eyes::~eyes(void)
{
  track->DieDieDie();
}
