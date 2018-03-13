//
//  DynMessage.cpp
//
//  Helpers for Dynamixel motor control

//#pragma once

#include "stdafx.h"
#include "DynMessage.h"

using namespace std;

DynMessage::DynMessage(DynInstruction instruction, DynRegister reg, unsigned __int8 ID, unsigned __int16 param) :
_instruction(instruction),
_register(reg),
_deviceID(ID),
_param(param)
{
	_paramLength = paramLengthForRegister(reg);
}

//#pragma mark - Convenience message generators

vector<unsigned __int8> DynMessage::SetPosition(unsigned __int16 position, unsigned __int8 device)
{
	return DynMessage(kDYN_Write, kDYN_GoalPosition).setParam(position).setDevice(device).get();
}

vector<unsigned __int8> DynMessage::GetPosition(unsigned __int8 device)
{
	return DynMessage(kDYN_Read, kDYN_PresentPosition, device).get();
}

vector<unsigned __int8> DynMessage::SetClockwiseLimit(unsigned __int16 limit, unsigned __int8 device)
{
	return DynMessage(kDYN_Write, kDYN_CWAngleLimit).setParam(limit).setDevice(device).get();
}

vector<unsigned __int8> DynMessage::SetCounterClockwiseLimit(unsigned __int16 limit, unsigned __int8 device)
{
	return DynMessage(kDYN_Write, kDYN_CCWAngleLimit).setParam(limit).setDevice(device).get();
}

vector<unsigned __int8> DynMessage::SetSpeed(unsigned __int16 speed, unsigned __int8 device)
{
	return DynMessage(kDYN_Write, kDYN_MotorSpeed).setParam(speed).setDevice(device).get();
}

vector<unsigned __int8> DynMessage::GetSpeed(unsigned __int8 device)
{
	return DynMessage(kDYN_Read, kDYN_PresentSpeed, device).get();
}

vector<unsigned __int8> DynMessage::SetEnable(unsigned __int8 enable, unsigned __int8 device)
{
	return DynMessage(kDYN_Write, kDYN_TorqueEnable).setParam(enable).setDevice(device).get();
}

vector<unsigned __int8> DynMessage::GetEnable(unsigned __int8 device)
{
	return DynMessage(kDYN_Read, kDYN_TorqueEnable, device).get();
}

vector<unsigned __int8> DynMessage::SetStatus(unsigned __int8 status, unsigned __int8 device)
{
	return DynMessage(kDYN_Write, kDYN_StatusReturnLevel).setParam(status).setDevice(device).get();
}

vector<unsigned __int8> DynMessage::SetCW_CM(unsigned __int8 cm, unsigned __int8 device)
{
	return DynMessage(kDYN_Write, kDYN_CWComplianceMargin).setParam(cm).setDevice(device).get();
}

vector<unsigned __int8> DynMessage::SetCCW_CM(unsigned __int8 cm, unsigned __int8 device)
{
	return DynMessage(kDYN_Write, kDYN_CCWComplianceMargin).setParam(cm).setDevice(device).get();
}

vector<unsigned __int8> DynMessage::SetCW_Slope(unsigned __int8 slope, unsigned __int8 device)
{
	return DynMessage(kDYN_Write, kDYN_CWComplianceSlope).setParam(slope).setDevice(device).get();
}

vector<unsigned __int8> DynMessage::SetCCW_Slope(unsigned __int8 slope, unsigned __int8 device)
{
	return DynMessage(kDYN_Write, kDYN_CCWComplianceSlope).setParam(slope).setDevice(device).get();
}

vector<unsigned __int8> DynMessage::SetPunch(unsigned __int8 punch, unsigned __int8 device)
{
	return DynMessage(kDYN_Write, kDYN_Punch).setParam(punch).setDevice(device).get();
}

vector<unsigned __int8> DynMessage::GetPunch(unsigned __int8 device)
{
	return DynMessage(kDYN_Read, kDYN_Punch, device).get();
}

vector<unsigned __int8> DynMessage::GetCW_CM(unsigned __int8 device)
{
	return DynMessage(kDYN_Read, kDYN_CWComplianceMargin, device).get();
}

vector<unsigned __int8> DynMessage::GetCCW_CM(unsigned __int8 device)
{
	return DynMessage(kDYN_Read, kDYN_CCWComplianceMargin, device).get();
}

vector<unsigned __int8> DynMessage::GetCW_Slope(unsigned __int8 device)
{
	return DynMessage(kDYN_Read, kDYN_CWComplianceSlope, device).get();
}

vector<unsigned __int8> DynMessage::GetCCW_Slope(unsigned __int8 device)
{
	return DynMessage(kDYN_Read, kDYN_CCWComplianceSlope, device).get();
}

vector<unsigned __int8> DynMessage::SetReturnDelay(unsigned __int8 delay, unsigned __int8 device)
{
	return DynMessage(kDYN_Write, kDYN_ReturnDelayTime).setParam(delay).setDevice(device).get();
}

vector<unsigned __int8> DynMessage::GetMoving(unsigned __int8 device)
{
	return DynMessage(kDYN_Read, kDYN_IsMoving, device).get();
}

vector<unsigned __int8> DynMessage::Reset(unsigned __int8 device)
{
	return DynMessage(kDYN_Reset).setDevice(device).get();
}

//#pragma mark - Accessors

DynMessage& DynMessage::setInstruction(DynInstruction instruction)
{
	_instruction = instruction;
	return *this;
}

DynMessage& DynMessage::setDevice(unsigned __int8 deviceID)
{
	_deviceID = deviceID;
	return *this;
}

DynMessage& DynMessage::setRegister(DynRegister reg)
{
	_register    = reg;
	_paramLength = paramLengthForRegister(reg);
	return *this;
}

DynMessage& DynMessage::setParam(unsigned __int16 param)
{
	_param = param;
	return *this;
}

//#pragma mark - Message generation

vector<unsigned __int8> DynMessage::get() const
{
	vector<unsigned __int8> msg;
	
	if(!this->isValid()) return msg; // Return blank message if invalid
	
	msg.push_back(kDYN_HeaderByte);
	msg.push_back(kDYN_HeaderByte);
	msg.push_back(_deviceID);
	addLengthByte(msg);
	msg.push_back(_instruction);
	msg.push_back(_register);
	addParams(msg);
	addChecksum(msg);

	return msg;
}

void DynMessage::addLengthByte(vector<unsigned __int8> &packet) const
{
	if(_instruction == kDYN_Read)
	{
		packet.push_back(0x04);
	}
	else if (_instruction == kDYN_Write ||
			 _instruction == kDYN_RegWrite)
	{
		packet.push_back(_paramLength + 3);
	}
	else
	{
		packet.push_back(0x02);
	}
}

void DynMessage::addParams(vector<unsigned __int8> &packet) const
{
	if(_instruction == kDYN_Read)
	{
		packet.push_back((unsigned __int8) _paramLength);
	}
	else if(_instruction == kDYN_Write || _instruction == kDYN_RegWrite)
	{
		if(_paramLength == OneByteParam)
		{
			packet.push_back((unsigned __int8) _param);
		}
		else if(_paramLength == TwoByteParam)
		{
			packet.push_back((unsigned __int8) _param);
			packet.push_back((unsigned __int8) (_param >> 8));
		}
	}
}

void DynMessage::addChecksum(vector<unsigned __int8> &packet) const
{
	unsigned __int8 checksum = 0;
	for(unsigned int i = 2; i < packet.size(); i++)
	{
		checksum += packet[i];
	}
	packet.push_back(~checksum);
}

//#pragma mark - Private util

DynMessage::ParamLength DynMessage::paramLengthForRegister(DynRegister address)
{
	if     (address < 0x02) return TwoByteParam;
	else if(address < 0x06) return OneByteParam;
	else if(address < 0x0B) return TwoByteParam;
	else if(address < 0x0E) return OneByteParam;
	else if(address < 0x10) return TwoByteParam;
	else if(address < 0x1E) return OneByteParam;
	else if(address < 0x2A) return TwoByteParam;
	else if(address < 0x2F) return OneByteParam;
	else return TwoByteParam;
}

bool DynMessage::isValid() const
{
	// Ping, Action and Reset require no extra info
	if(_instruction == kDYN_Ping ||
	   _instruction == kDYN_Action ||
	   _instruction == kDYN_Reset)
	{
		return true;
	}
	// Everything else requires a register to be specified
	else if (_register == kDYN_RegisterNotSet)
	{
		return false;
	}
	else
	{
		return true;
	}
}

//#pragma mark - Parsing status packets

unsigned __int8 DynParser::getDevice(const vector<unsigned __int8> &statusPacket)
{
	if(statusPacket.size() < 3) return 0;
	
	return statusPacket[2];
}


unsigned __int16 DynParser::getParam(const vector<unsigned __int8> &statusPacket)
{
	if(statusPacket.size() < 6) return 0;
	
	unsigned __int16 param = statusPacket[5];
	
	// if this is a two byte param, add the second byte
	if(statusPacket.size() > 7)
	{
		param += ((unsigned __int16)statusPacket[6]) << 8;
	}
	
	return param;
}

bool DynParser::isValid(const vector<unsigned __int8> &statusPacket)
{
	// test size
	if(statusPacket.size() < 5) return false;
	
	// test header
	if(statusPacket[0] != kDYN_HeaderByte ||
	   statusPacket[1] != kDYN_HeaderByte)
	{
		return false;
	}
	
	// test checksum
	unsigned __int8 expectedChecksum = 0;
	for(unsigned int i = 2; i < statusPacket.size()-1; i++)
	{
		expectedChecksum += statusPacket[i];
	}
	expectedChecksum = ~expectedChecksum;
	
	if(expectedChecksum != statusPacket.back())
	{
		return false;
	}
	
	return true;
}
