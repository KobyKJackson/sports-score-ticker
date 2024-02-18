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
  mID(aID), mUpdateTimestamp(chrono::steady_clock::now()), mXPosition(0), mYPosition(0)
{
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

void ObjectGroupClass::AddObject(ObjectTypeClass *&aObject)
{
	this->mObjects.push_back(aObject);
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
