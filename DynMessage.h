//
//  DynMessage.h
//
//  Created by Adam on 12-07-30.
//  Copyright (c) 2012 Aesthetec. All rights reserved.
//

#ifndef __DynMessage__
#define __DynMessage__

#include <vector>

// Registers (page 20)
enum DynRegister
{
  // EEPROM
  kDYN_ModelNumber           = 0x00,
  kDYN_FirmwareVersion       = 0x02,
  kDYN_DeviceID              = 0x03,
  kDYN_BaudRate              = 0x04,
  kDYN_ReturnDelayTime       = 0x05,
  kDYN_CWAngleLimit          = 0x06,
  kDYN_CCWAngleLimit         = 0x08,
  kDYN_TemperatureLimit      = 0x0B,
  kDYN_VoltageMinLimit       = 0x0C,
  kDYN_VoltageMaxLimit       = 0x0D,
  kDYN_MaxTorque             = 0x0E,
  kDYN_StatusReturnLevel     = 0x10,
  kDYN_AlarmLED              = 0x11,
  kDYN_AlarmShutdown         = 0x12,
  
  // RAM
  kDYN_TorqueEnable          = 0x18,
  kDYN_LED                   = 0x19,
  kDYN_CWComplianceMargin    = 0x1A,
  kDYN_CCWComplianceMargin   = 0x1B,
  kDYN_CWComplianceSlope     = 0x1C,
  kDYN_CCWComplianceSlope    = 0x1D,
  kDYN_GoalPosition          = 0x1E,
  kDYN_MotorSpeed            = 0x20,
  kDYN_TorqueLimit           = 0x22,
  kDYN_PresentPosition       = 0x24,
  kDYN_PresentSpeed          = 0x26,
  kDYN_PresentLoad           = 0x28,
  kDYN_PresentVoltage        = 0x2A,
  kDYN_PresentTemperature    = 0x2B,
  kDYN_RegisteredInstruction = 0x2C,
  kDYN_IsMoving              = 0x2E,
  kDYN_LockEEPROM            = 0x2F,
  kDYN_Punch                 = 0x30,
  
  kDYN_RegisterNotSet        = 0xFF
};

// Instructions (page 16)
enum DynInstruction
{
  kDYN_Ping      = 0x01,
  kDYN_Read      = 0x02,
  kDYN_Write     = 0x03,
  kDYN_RegWrite  = 0x04,
  kDYN_Action    = 0x05,
  kDYN_Reset     = 0x06,
//  kDYN_SyncWrite = 0x83, Sync Write not implemented
  
  kDYN_InstructionNotSet = 0xFF
};

// Error types (page 18)
enum DynError
{
  kDYN_Error_Instruction,
  kDYN_Error_Overload,
  kDYN_Error_Checksum,
  kDYN_Error_Range,
  kDYN_Error_Overheat,
  kDYN_Error_AngleLimit,
  kDYN_Error_Voltage,
  kDYN_InvalidStatusPacket
};

static const unsigned __int8 kDYN_HeaderByte  = 0xFF;

// Messages sent to the broadcast ID will be received by every device.
// No status packets will be sent back when the broadcast ID is used.
static const unsigned __int8 kDYN_BroadcastID = 0xFE;

// The DynMessage class handles RX-64 message creation. You can either create a
// message by creating a DynMessage object and calling get() on it, or by using
// one of the message generators.

// Here are some examples showing how to create a message which sets device 5's
// goal position to 0x123

// Using a message generator
// -------------------------------------------------------------------
// vector<unsigned __int8> msg = DynMessage::SetPosition(0x123, 5);
// -------------------------------------------------------------------

// Using a DynMessage object directly
// -------------------------------------------------------------------
// DynMessage m = DynMessage(kDYN_Write, kDYN_GoalPosition, 5, 0x123);
// vector<unsigned __int8> msg = m.get();
// -------------------------------------------------------------------
// or
// -------------------------------------------------------------------
// DynMessage m;
// m.setInstruction(kDYN_Write);
// m.setRegister(kDYN_GoalPostion);
// m.setDevice(5);
// m.setParam(0x123);
// vector<unsigned __int8> msg = m.get();
// -------------------------------------------------------------------

// DynMessage keeps track of how many parameter bytes an instruction should have.
// Setting a param value to "5" when the register expects a 2-byte param will
// still work.

class DynMessage
{
public:
  explicit DynMessage(DynInstruction inst  = kDYN_InstructionNotSet,
                      DynRegister    reg   = kDYN_RegisterNotSet,
                      unsigned __int8        ID    = kDYN_BroadcastID,
                      unsigned __int16       param = 0);
  
  DynMessage& setInstruction(DynInstruction instruction);
  DynMessage& setDevice(unsigned __int8 deviceID);
  DynMessage& setRegister(DynRegister reg);
  DynMessage& setParam(unsigned __int16 param);
  
  std::vector<unsigned __int8> get() const;
  
  static std::vector<unsigned __int8> SetPosition(unsigned __int16 position, unsigned __int8 device);
  static std::vector<unsigned __int8> GetPosition(unsigned __int8 device);
  static std::vector<unsigned __int8> SetClockwiseLimit(unsigned __int16 limit, unsigned __int8 device);
  static std::vector<unsigned __int8> SetCounterClockwiseLimit(unsigned __int16 limit, unsigned __int8 device);
  static std::vector<unsigned __int8> SetSpeed(unsigned __int16 speed, unsigned __int8 device);
  static std::vector<unsigned __int8> GetSpeed(unsigned __int8 device);
  static std::vector<unsigned __int8> SetStatus(unsigned __int8 status, unsigned __int8 device);
  static std::vector<unsigned __int8> SetEnable(unsigned __int8 enable, unsigned __int8 device);
  static std::vector<unsigned __int8> GetEnable(unsigned __int8 device);
  static std::vector<unsigned __int8> SetCW_CM(unsigned __int8 cm, unsigned __int8 device);
  static std::vector<unsigned __int8> SetCCW_CM(unsigned __int8 cm, unsigned __int8 device);
  static std::vector<unsigned __int8> SetCW_Slope(unsigned __int8 slope, unsigned __int8 device);
  static std::vector<unsigned __int8> SetCCW_Slope(unsigned __int8 slope, unsigned __int8 device);
  static std::vector<unsigned __int8> GetCW_CM(unsigned __int8 device);
  static std::vector<unsigned __int8> GetCCW_CM(unsigned __int8 device);
  static std::vector<unsigned __int8> GetCW_Slope(unsigned __int8 device);
  static std::vector<unsigned __int8> GetCCW_Slope(unsigned __int8 device);
  static std::vector<unsigned __int8> SetPunch(unsigned __int8 punch, unsigned __int8 device);
  static std::vector<unsigned __int8> GetPunch(unsigned __int8 device);
  static std::vector<unsigned __int8> GetMoving(unsigned __int8 device);
  static std::vector<unsigned __int8> SetReturnDelay(unsigned __int8 delay, unsigned __int8 device);
  static std::vector<unsigned __int8> Reset(unsigned __int8 device);
  
private:
  unsigned __int8  _deviceID;
  unsigned __int8  _instruction;
  unsigned __int8  _register;
  unsigned __int16 _param;
  
  enum ParamLength {NoParam = 0, OneByteParam = 1, TwoByteParam = 2, FourByteParam = 3};
  
  ParamLength _paramLength;
  static ParamLength paramLengthForRegister(DynRegister address);
  bool isValid() const;
  
  void addParams(std::vector<unsigned __int8> &packet)     const;
  void addLengthByte(std::vector<unsigned __int8> &packet) const;
  void addChecksum(std::vector<unsigned __int8> &packet)   const;
};

// These functions can be used to parse status packets returned from an RX-64.

// Example:

// vector<unsigned __int8> receivedPacket;
// if(DynParser::isValid(receivedPacket))
// {
//     unsigned __int16 returnedValue = DynParser::getParam(receivedPacket);
// }

struct DynParser
{
  static bool isValid(const std::vector<unsigned __int8> &statusPacket);
  static unsigned __int8 getDevice(const std::vector<unsigned __int8> &statusPacket);
  static unsigned __int16 getParam(const std::vector<unsigned __int8> &statusPacket);
};

#endif /* defined(__DynMessage__) */
