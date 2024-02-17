/*******************************************************************************
** @file       BaseObject.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "BaseObject.h"

/* Exported Data -------------------------------------------------------------*/

/* Static Class Member Initialization ----------------------------------------*/

/* Class Constructors --------------------------------------------------------*/
BaseObjectClass::BaseObjectClass(BASE_OBJECT_TYPE aObjectType, std::vector<uint8_t> aLocation, std::string aValue) :
  mObjectType(aObjectType),
  mValue(aValue),
  mYPosition((aLocation[0] - 1) * ROW_SIZE),
  mHeight(aLocation.size() * ROW_SIZE)
{
}

/* Class Destructor ----------------------------------------------------------*/
BaseObjectClass::~BaseObjectClass()
{
}

/* Public Static Class Methods -----------------------------------------------*/
BASE_OBJECT_TYPE BaseObjectClass::StringTypeToEnumType(const std::string &aValue)
{
	if (aValue == "text")
	{
		return BASE_OBJECT_TYPE::TEXT;
	}
	else if (aValue == "image")
	{
		return BASE_OBJECT_TYPE::IMAGE;
	}
	else
	{
		return BASE_OBJECT_TYPE::UNKNOWN;
	}
}
/* Public Class Methods ------------------------------------------------------*/
BASE_OBJECT_TYPE BaseObjectClass::GetBaseObjectType() const
{
	return this->mObjectType;
}

std::string BaseObjectClass::GetValue()
{
	return this->mValue;
}
uint8_t BaseObjectClass::GetHeight()
{
	return this->mHeight;
}
/* Protected Static Class Methods --------------------------------------------*/

/* Protected Class Methods ---------------------------------------------------*/

/* Private Static Class Methods ----------------------------------------------*/

/* Private Class Methods -----------------------------------------------------*/
