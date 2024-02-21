/*******************************************************************************
** @file       MultiObject.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "MultiObject.h"

#include <algorithm>
#include "BaseObject.h"

using namespace std;
/* Exported Data -------------------------------------------------------------*/

/* Static Class Member Initialization ----------------------------------------*/

/* Class Constructors --------------------------------------------------------*/
MultiObjectClass::MultiObjectClass() : ObjectTypeClass()
{
}

/* Class Destructor ----------------------------------------------------------*/
MultiObjectClass::~MultiObjectClass()
{
	for (ObjectTypeClass *lpObject : this->mObjects)
	{
		delete lpObject;
	}
}

/* Public Virtual Class Methods ----------------------------------------------*/
OBJECT_TYPE MultiObjectClass::GetObjectType() const
{
	return OBJECT_TYPE::MULTI;
};

MultiObjectClass *MultiObjectClass::clone() const
{
	MultiObjectClass *lpMultiObjectClass = new MultiObjectClass(*this);

	lpMultiObjectClass->RemoveAllObjects();

	for (uint8_t i = 0; i < this->mObjects.size(); i++)
	{
		lpMultiObjectClass->AddObject(this->mObjects[i]->clone());
	}

	return lpMultiObjectClass;
}
/* Public Static Class Methods -----------------------------------------------*/

/* Public Class Methods ------------------------------------------------------*/
void MultiObjectClass::AddObject(ObjectTypeClass *aObject)
{
	this->mObjects.push_back(aObject);
	this->calculateLength();
}

void MultiObjectClass::RemoveAllObjects()
{
	this->mObjects.clear();
}

ObjectTypeClass *MultiObjectClass::GetByIndex(size_t aIndex)
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

void MultiObjectClass::SetXPosition(int aValue)
{
	this->mXPosition = aValue;
	for (const auto &lpObject : this->mObjects)
	{
		lpObject->SetXPosition(this->mXPosition);
	}
}

/* Protected Static Class Methods --------------------------------------------*/

/* Protected Class Methods ---------------------------------------------------*/

/* Private Virtual Class Methods ----------------------------------------------*/
void MultiObjectClass::calculateLength()
{
	uint32_t lLength = 0;
	for (const auto &lpObject : this->mObjects)
	{
		if (lpObject->GetLength() > lLength)
		{
			lLength = lpObject->GetLength();
		}
	}
	this->mLength = lLength;
}
/* Private Static Class Methods ----------------------------------------------*/

/* Private Class Methods -----------------------------------------------------*/
