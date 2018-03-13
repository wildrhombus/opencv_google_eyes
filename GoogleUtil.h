
#ifndef Google_GoogleUtil_h
#define Google_GoogleUtil_h

// These constants are meant to be used in conjunction with DynMessage

// eg:

// msg = DynMessage::SetClockwiseLimit(kGoogleLimit_CW);
// msg = DynMessage::SetPosition(kGooglePosition_Centre, kGoogleEye_Right);

// Note that omitting the device ID param will use the broadcast ID, so...

// msg = DynMessage::SetPosition(kGooglePosition_Centre);

// ...will centre both eyes

static const unsigned __int8 kGoogleEye_Left  = 2;
static const unsigned __int8 kGoogleEye_Right = 1;

static const unsigned __int16 kGoogleLimit_CW  = 0x0FF;
static const unsigned __int16 kGoogleLimit_CCW = 0x2FF;

static const unsigned __int16 kGooglePosition_Left   = kGoogleLimit_CW;
static const unsigned __int16 kGooglePosition_Right  = kGoogleLimit_CCW;
static const unsigned __int16 kGooglePosition_Centre = 0x200;

#endif
