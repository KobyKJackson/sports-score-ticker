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
  mXPosition(0),
  mLength(0)
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

/* Protected Static Class Methods --------------------------------------------*/

/* Protected Class Methods ---------------------------------------------------*/

/* Private Static Class Methods ----------------------------------------------*/

/* Private Class Methods -----------------------------------------------------*/
