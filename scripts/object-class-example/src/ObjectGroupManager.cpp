/*******************************************************************************
** @file       ObjectGroupManager.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "ObjectGroupManager.h"

#include "ObjectGroup.h"

/* Exported Data -------------------------------------------------------------*/

/* Static Class Member Initialization ----------------------------------------*/

/* Class Constructors --------------------------------------------------------*/
ObjectGroupManagerClass::ObjectGroupManagerClass()
{
}

/* Class Destructor ----------------------------------------------------------*/
ObjectGroupManagerClass::~ObjectGroupManagerClass()
{
}

/* Public Static Class Methods -----------------------------------------------*/

/* Public Class Methods ------------------------------------------------------*/
void ObjectGroupManagerClass::AddOrUpdate(ObjectGroupClass *&aObjectGroup)
{
	bool found = false;
	for (auto &lpObject : this->mObjectGroups)
	{
		if (lpObject->GetID() == aObjectGroup->GetID())
		{
			lpObject = aObjectGroup;
			found = true;
			break;
		}
	}

	if (!found)
	{
		this->mObjectGroups.push_back(aObjectGroup);
	}
}

bool ObjectGroupManagerClass::RemoveByID(std::string aID)
{
	for (auto it = this->mObjectGroups.begin(); it != this->mObjectGroups.end(); ++it)
	{
		if ((*it)->GetID() == aID)
		{
			this->mObjectGroups.erase(it);
			return true;
		}
	}
	return false;
}

ObjectGroupClass *ObjectGroupManagerClass::GetByID(std::string aID)
{
	for (const auto &lObjectGroup : this->mObjectGroups)
	{
		if (lObjectGroup->GetID() == aID)
		{
			return lObjectGroup;
		}
	}
	return nullptr;
}

ObjectGroupClass *ObjectGroupManagerClass::GetByIndex(size_t aIndex)
{
	if (aIndex < this->mObjectGroups.size())
	{
		return this->mObjectGroups[aIndex];
	}
	else
	{
		return nullptr;
	}
}

size_t ObjectGroupManagerClass::GetNumberOfObjectGroups()
{
	return this->mObjectGroups.size();
}

/* Protected Static Class Methods --------------------------------------------*/

/* Protected Class Methods ---------------------------------------------------*/

/* Private Static Class Methods ----------------------------------------------*/

/* Private Class Methods -----------------------------------------------------*/
