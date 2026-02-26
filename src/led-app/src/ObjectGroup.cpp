/*******************************************************************************
** @file       ObjectGroup.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "ObjectGroup.h"

#include "DisplayObject.h"

using namespace std;

/* Class Constructors --------------------------------------------------------*/
ObjectGroupClass::ObjectGroupClass(string aID) : mID(aID), mUpdateTimestamp(chrono::steady_clock::now()), mLength(0), mXPosition(0), mYPosition(0)
{
}

ObjectGroupClass::ObjectGroupClass(const ObjectGroupClass &other) : mID(other.mID), mLength(other.mLength), mXPosition(other.mXPosition), mYPosition(other.mYPosition), mUpdateTimestamp(other.mUpdateTimestamp)
{
	for (const auto &obj : other.mObjects)
	{
		mObjects.push_back(obj->clone());
	}
}

/* Class Destructor ----------------------------------------------------------*/
ObjectGroupClass::~ObjectGroupClass()
{
	for (DisplayObjectClass *lpObject : this->mObjects)
	{
		delete lpObject;
	}
}

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
	this->updateChildrenXPosition();
}

void ObjectGroupClass::IncrementXPosition()
{
	this->mXPosition--;
	this->updateChildrenXPosition();
}

void ObjectGroupClass::AddObject(DisplayObjectClass *&aObject)
{
	this->mObjects.push_back(aObject);
	this->calculateLength();
}

void ObjectGroupClass::RemoveAllObjects()
{
	for (DisplayObjectClass *lpObject : this->mObjects)
	{
		delete lpObject;
	}
	this->mObjects.clear();
}

DisplayObjectClass *ObjectGroupClass::GetByIndex(size_t aIndex)
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

std::vector<DisplayObjectClass *> ObjectGroupClass::GetObjects()
{
	return this->mObjects;
}

chrono::time_point<chrono::steady_clock> ObjectGroupClass::GetUpdateTimestamp()
{
	return this->mUpdateTimestamp;
}

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

void ObjectGroupClass::updateChildrenXPosition()
{
	uint32_t lRunningXPosition = this->mXPosition;
	for (const auto &lpObject : this->mObjects)
	{
		lpObject->SetXPosition(lRunningXPosition);
		lRunningXPosition += lpObject->GetLength();
	}
}
