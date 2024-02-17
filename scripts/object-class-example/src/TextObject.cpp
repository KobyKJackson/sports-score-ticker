/*******************************************************************************
** @file       TextObject.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "TextObject.h"

/* Exported Data -------------------------------------------------------------*/

/* Static Class Member Initialization ----------------------------------------*/

/* Class Constructors --------------------------------------------------------*/
TextObjectClass::TextObjectClass(std::vector<uint8_t> aLocation, std::string aValue) :
  ObjectTypeClass(), BaseObjectClass(BASE_OBJECT_TYPE::TEXT, aLocation, aValue)
{
}

/* Class Destructor ----------------------------------------------------------*/
TextObjectClass::~TextObjectClass()
{
}

/* Public Virtual Class Methods ----------------------------------------------*/
OBJECT_TYPE TextObjectClass::GetObjectType() const
{
	return OBJECT_TYPE::TEXT;
};
/* Public Static Class Methods -----------------------------------------------*/

/* Public Class Methods ------------------------------------------------------*/

/* Protected Static Class Methods --------------------------------------------*/

/* Protected Class Methods ---------------------------------------------------*/

/* Private Static Class Methods ----------------------------------------------*/

/* Private Class Methods -----------------------------------------------------*/
