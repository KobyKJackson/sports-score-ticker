/*******************************************************************************
** @file       MultiObject.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "MultiObject.h"

using namespace std;

/* Class Constructors --------------------------------------------------------*/
MultiObjectClass::MultiObjectClass() : DisplayObjectClass()
{
}

/* Class Destructor ----------------------------------------------------------*/
MultiObjectClass::~MultiObjectClass()
{
	for (DisplayObjectClass *lpObject : this->mObjects)
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
	MultiObjectClass *lpMultiObjectClass = new MultiObjectClass();

	for (size_t i = 0; i < this->mObjects.size(); i++)
	{
		lpMultiObjectClass->AddObject(this->mObjects[i]->clone());
	}

	return lpMultiObjectClass;
}

/* Public Class Methods ------------------------------------------------------*/
void MultiObjectClass::AddObject(DisplayObjectClass *aObject)
{
	this->mObjects.push_back(aObject);
	this->calculateLength();
}

void MultiObjectClass::RemoveAllObjects()
{
	for (DisplayObjectClass *lpObject : this->mObjects)
	{
		delete lpObject;
	}
	this->mObjects.clear();
}

DisplayObjectClass *MultiObjectClass::GetByIndex(size_t aIndex)
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
