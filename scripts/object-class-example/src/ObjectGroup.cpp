/*******************************************************************************
** @file       ObjectGroup.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "ObjectGroup.h"

#include "ObjectType.h"

using namespace std;

/* Exported Data -------------------------------------------------------------*/

/* Static Class Member Initialization ----------------------------------------*/

/* Class Constructors --------------------------------------------------------*/
ObjectGroupClass::ObjectGroupClass(string aID) :
  mID(aID), mUpdateTimestamp(chrono::steady_clock::now()), mLength(0), mXPosition(0), mYPosition(0)
{
}

ObjectGroupClass::ObjectGroupClass(const ObjectGroupClass &other) :
  mID(other.mID), mLength(other.mLength), mXPosition(other.mXPosition), mYPosition(other.mYPosition), mUpdateTimestamp(other.mUpdateTimestamp)
{
	for (const auto &obj : other.mObjects)
	{
		mObjects.push_back(obj->clone()); // Use clone to deep copy each object
	}
}
/* Class Destructor ----------------------------------------------------------*/
ObjectGroupClass::~ObjectGroupClass()
{
}

/* Public Static Class Methods -----------------------------------------------*/

/* Public Class Methods ------------------------------------------------------*/
string ObjectGroupClass::GetID() const
{
	return this->mID;
}

uint32_t ObjectGroupClass::GetLength() const
{
	return this->mLength;
}

uint32_t ObjectGroupClass::GetXPosition() const
{
	return this->mXPosition;
}

void ObjectGroupClass::SetXPosition(uint32_t aValue)
{
	this->mXPosition = aValue;
}

void ObjectGroupClass::AddObject(ObjectTypeClass *&aObject)
{
	this->mObjects.push_back(aObject);
}

void ObjectGroupClass::RemoveAllObjects()
{
	for (ObjectTypeClass *lpObject : this->mObjects)
	{
		delete lpObject;
	}

	this->mObjects.clear();
}

ObjectTypeClass *ObjectGroupClass::GetByIndex(size_t aIndex)
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

size_t ObjectGroupClass::GetNumberOfObjects()
{
	return this->mObjects.size();
}

chrono::time_point<chrono::steady_clock> ObjectGroupClass::GetUpdateTimestamp()
{
	return this->mUpdateTimestamp;
}

/* Protected Static Class Methods --------------------------------------------*/

/* Protected Class Methods ---------------------------------------------------*/

/* Private Static Class Methods ----------------------------------------------*/

/* Private Class Methods -----------------------------------------------------*/
void ObjectGroupClass::calculateLength()
{
	uint32_t lTotalWidth = 0;
	for (const auto &lpObject : this->mObjects)
	{
		lTotalWidth += lpObject->GetLength();
	}
	this->mLength = lTotalWidth;
}