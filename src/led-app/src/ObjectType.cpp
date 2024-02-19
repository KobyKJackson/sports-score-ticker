/*******************************************************************************
** @file       ObjectType.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "ObjectType.h"

/* Exported Data -------------------------------------------------------------*/

/* Static Class Member Initialization ----------------------------------------*/

/* Class Constructors --------------------------------------------------------*/
ObjectTypeClass::ObjectTypeClass() :
  mLength(0),
  mXPosition(0)
{
}

/* Class Destructor ----------------------------------------------------------*/
ObjectTypeClass::~ObjectTypeClass()
{
}

/* Public Static Class Methods -----------------------------------------------*/
OBJECT_TYPE ObjectTypeClass::StringTypeToEnumType(const std::string &aValue)
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
uint32_t ObjectTypeClass::GetLength() const
{
	return this->mLength;
}

void ObjectTypeClass::SetXPosition(uint32_t aValue)
{
	this->mXPosition = aValue;
}

/* Protected Static Class Methods --------------------------------------------*/

/* Protected Class Methods ---------------------------------------------------*/

/* Private Static Class Methods ----------------------------------------------*/

/* Private Class Methods -----------------------------------------------------*/
