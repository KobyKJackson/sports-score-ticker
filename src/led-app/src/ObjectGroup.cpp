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
ObjectGroupClass::ObjectGroupClass(string aID) : mID(aID), mUpdateTimestamp(chrono::steady_clock::now()), mLength(0), mXPosition(0), mYPosition(0)
{
}

ObjectGroupClass::ObjectGroupClass(const ObjectGroupClass &other) : mID(other.mID), mLength(other.mLength), mXPosition(other.mXPosition), mYPosition(other.mYPosition), mUpdateTimestamp(other.mUpdateTimestamp)
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

int ObjectGroupClass::GetLength() const
{
	return this->mLength;
}

int ObjectGroupClass::GetXPosition() const
{
	return this->mXPosition;
}

void ObjectGroupClass::SetXPosition(int aValue)
{
	this->mXPosition = aValue;
	this->updateChilderenXPosition();
}

void ObjectGroupClass::IncrementXPosition()
{
	this->mXPosition--;
	this->updateChilderenXPosition();
}

void ObjectGroupClass::AddObject(ObjectTypeClass *&aObject)
{
	this->mObjects.push_back(aObject);
	this->calculateLength();
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

std::vector<ObjectTypeClass *> ObjectGroupClass::GetObjects()
{
	return this->mObjects;
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
	uint32_t lTotalLength = 0;
	for (const auto &lpObject : this->mObjects)
	{
		lTotalLength += lpObject->GetLength();
	}
	this->mLength = lTotalLength;
}

void ObjectGroupClass::updateChilderenXPosition()
{
	uint32_t lRunningXPosition = this->mXPosition;
	for (const auto &lpObject : this->mObjects)
	{
		lpObject->SetXPosition(lRunningXPosition);
		lRunningXPosition += lpObject->GetLength();
	}
}
