/*******************************************************************************
** @file       MultiObject.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "MultiObject.h"

#include "BaseObject.h"

/* Exported Data -------------------------------------------------------------*/

/* Static Class Member Initialization ----------------------------------------*/

/* Class Constructors --------------------------------------------------------*/
MultiObjectClass::MultiObjectClass() :
  ObjectTypeClass()
{
}

/* Class Destructor ----------------------------------------------------------*/
MultiObjectClass::~MultiObjectClass()
{
}

/* Public Virtual Class Methods ----------------------------------------------*/
OBJECT_TYPE MultiObjectClass::GetObjectType() const
{
	return OBJECT_TYPE::MULTI;
};

/* Public Static Class Methods -----------------------------------------------*/

/* Public Class Methods ------------------------------------------------------*/
void MultiObjectClass::AddObject(BaseObjectClass *&aObject)
{
	this->mObjects.push_back(aObject);
}

BaseObjectClass *MultiObjectClass::GetByIndex(size_t aIndex)
{
	if (aIndex < this->mObjects.size())
	{
		return this->mObjects[aIndex];
	}
	else
	{
		return nullptr;
	}
}

size_t MultiObjectClass::GetNumberOfObjects()
{
	return this->mObjects.size();
}

/* Protected Static Class Methods --------------------------------------------*/

/* Protected Class Methods ---------------------------------------------------*/

/* Private Static Class Methods ----------------------------------------------*/

/* Private Class Methods -----------------------------------------------------*/
