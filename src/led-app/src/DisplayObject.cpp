/*******************************************************************************
** @file       DisplayObject.cpp
** @brief      Unified base class for all displayable objects
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "DisplayObject.h"

/* Class Constructors --------------------------------------------------------*/
DisplayObjectClass::DisplayObjectClass() : mLength(0),
										   mXPosition(0),
										   mObjectType(OBJECT_TYPE::UNKNOWN),
										   mYPosition(0),
										   mHeight(0)
{
}

DisplayObjectClass::DisplayObjectClass(OBJECT_TYPE aObjectType, std::vector<uint8_t> aLocation, std::string aValue) : mLength(0),
																													  mXPosition(0),
																													  mObjectType(aObjectType),
																													  mValue(aValue),
																													  mYPosition((aLocation[0] - 1) * ROW_SIZE),
																													  mHeight(aLocation.size() * ROW_SIZE)
{
}

/* Class Destructor ----------------------------------------------------------*/
DisplayObjectClass::~DisplayObjectClass()
{
}

/* Public Static Class Methods -----------------------------------------------*/
OBJECT_TYPE DisplayObjectClass::StringTypeToEnumType(const std::string &aValue)
{
	if (aValue == "text")
	{
		return OBJECT_TYPE::TEXT;
	}
	else if (aValue == "image")
	{
		return OBJECT_TYPE::IMAGE;
	}
	else if (aValue == "multi")
	{
		return OBJECT_TYPE::MULTI;
	}
	else
	{
		return OBJECT_TYPE::UNKNOWN;
	}
}

/* Public Class Methods ------------------------------------------------------*/
uint32_t DisplayObjectClass::GetLength() const
{
	return this->mLength;
}

int DisplayObjectClass::GetXPosition() const
{
	return this->mXPosition;
}

void DisplayObjectClass::SetXPosition(int aValue)
{
	this->mXPosition = aValue;
}

std::string DisplayObjectClass::GetValue() const
{
	return this->mValue;
}

void DisplayObjectClass::SetValue(const std::string &aValue)
{
	this->mValue = aValue;
}

uint8_t DisplayObjectClass::GetYPosition() const
{
	return this->mYPosition;
}

uint8_t DisplayObjectClass::GetHeight() const
{
	return this->mHeight;
}
