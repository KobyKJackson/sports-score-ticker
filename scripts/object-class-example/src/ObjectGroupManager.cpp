/*******************************************************************************
** @file       ObjectGroupManager.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "ObjectGroupManager.h"

#include <fstream>
#include <iostream>
#include "BaseObject.h"
#include "ImageObject.h"
#include "MultiObject.h"
#include "ObjectGroup.h"
#include "ObjectType.h"
#include "TextObject.h"
#include "json.hpp"

using json = nlohmann::json;

using namespace std;
/* Exported Data -------------------------------------------------------------*/

/* Static Class Member Initialization ----------------------------------------*/

/* Class Constructors --------------------------------------------------------*/
ObjectGroupManagerClass::ObjectGroupManagerClass() :
  mIsThreadRunning(true)
{
	this->mThread = thread(&ObjectGroupManagerClass::threadFunction, this);
}

/* Class Destructor ----------------------------------------------------------*/
ObjectGroupManagerClass::~ObjectGroupManagerClass()
{
	this->mIsThreadRunning = false;
	if (this->mThread.joinable())
	{
		this->mThread.join();
	}
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

bool ObjectGroupManagerClass::RemoveByID(string aID)
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

ObjectGroupClass *ObjectGroupManagerClass::GetByID(string aID)
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

void ObjectGroupManagerClass::PrintAllObjects()
{
	for (uint8_t i = 0; i < this->GetNumberOfObjectGroups(); i++)
	{
		ObjectGroupClass *lpObjectGroup = this->GetByIndex(i);

		for (uint8_t j = 0; j < lpObjectGroup->GetNumberOfObjects(); j++)
		{
			ObjectTypeClass *lpObject = lpObjectGroup->GetByIndex(j);
			switch (lpObject->GetObjectType())
			{
				case OBJECT_TYPE::IMAGE:
				{
					ImageObjectClass *lpImageObject = static_cast<ImageObjectClass *>(lpObject);
					cout << "data: " << lpImageObject->GetValue() << endl;
				}
				break;

				case OBJECT_TYPE::TEXT:
				{
					TextObjectClass *lpTextObject = static_cast<TextObjectClass *>(lpObject);
					cout << "data: " << lpTextObject->GetValue() << endl;
				}
				break;

				case OBJECT_TYPE::MULTI:
				{
					MultiObjectClass *lpMultiObject = static_cast<MultiObjectClass *>(lpObject);
					for (uint8_t k = 0; k < lpMultiObject->GetNumberOfObjects(); k++)
					{
						ObjectTypeClass *lpObjectType = lpMultiObject->GetByIndex(k);
						switch (lpObjectType->GetObjectType())
						{
							case OBJECT_TYPE::IMAGE:
							{
								ImageObjectClass *lpImageObject = static_cast<ImageObjectClass *>(lpObjectType);
								cout << "data: " << lpImageObject->GetValue() << endl;
							}
							break;

							case OBJECT_TYPE::TEXT:
							{
								TextObjectClass *lpTextObject = static_cast<TextObjectClass *>(lpObjectType);
								cout << "data: " << lpTextObject->GetValue() << endl;
							}
							break;

							default:
								break;
						}
					}
				}
				break;

				default:
					break;
			}
		}
	}
}
/* Protected Static Class Methods --------------------------------------------*/

/* Protected Class Methods ---------------------------------------------------*/

/* Private Static Class Methods ----------------------------------------------*/

/* Private Class Methods -----------------------------------------------------*/
void ObjectGroupManagerClass::threadFunction()
{
	cout << "ObjectGroupManagerClass thread is running." << endl;
	while (this->mIsThreadRunning)
	{
		// Ping the server and update
		//Read JSON
		json lJSONData;
		try
		{
			ifstream f("../src/example.json");
			lJSONData = json::parse(f);
			cout << "Example element: " << lJSONData << endl;
		}
		catch (json::parse_error &e)
		{
			cerr << "JSON parse error: " << e.what() << endl;
		}

		//Update Objects
		for (const auto &lGame : lJSONData["games"])
		{
			string lID = lGame["id"];
			ObjectGroupClass *lpObjectGroup = this->GetByID(lID);

			if (lpObjectGroup != nullptr) //TODO: Update, do this better, right now I just delete everything and reconstruct on update
			{
				lpObjectGroup->RemoveAllObjects();
			}
			else // New Game
			{
				lpObjectGroup = new ObjectGroupClass(lID);
			}

			for (const auto &lElement : lGame["data"])
			{
				OBJECT_TYPE lType = ObjectTypeClass::StringTypeToEnumType(lElement["type"]);
				ObjectTypeClass *lpObject = nullptr;
				switch (lType)
				{
					case OBJECT_TYPE::IMAGE:
					{
						vector<uint8_t> lLocation = lElement["location"];
						string lData = lElement["data"];
						lpObject = new ImageObjectClass(lLocation, lData);
					}
					break;

					case OBJECT_TYPE::TEXT:
					{
						vector<uint8_t> lLocation = lElement["location"];
						string lData = lElement["data"];
						lpObject = new TextObjectClass(lLocation, lData);
					}
					break;

					case OBJECT_TYPE::MULTI:
					{
						lpObject = new MultiObjectClass();

						for (const auto &lElementType : lElement["data"])
						{
							OBJECT_TYPE lObjectType = ObjectTypeClass::StringTypeToEnumType(lElementType["type"]);
							ObjectTypeClass *lpObjectType = nullptr;
							switch (lObjectType)
							{
								case OBJECT_TYPE::IMAGE:
								{
									vector<uint8_t> lLocation = lElementType["location"];
									string lData = lElementType["data"];
									lpObjectType = new ImageObjectClass(lLocation, lData);
								}
								break;

								case OBJECT_TYPE::TEXT:
								{
									vector<uint8_t> lLocation = lElementType["location"];
									string lData = lElementType["data"];
									lpObjectType = new TextObjectClass(lLocation, lData);
								}
								break;

								default:
									break;
							}
							(static_cast<MultiObjectClass *>(lpObject))->AddObject(lpObjectType);
						}
					}
					break;

					default:
						break;
				}
				if (lpObject != nullptr)
				{
					lpObjectGroup->AddObject(lpObject);
				}
			}
			this->AddOrUpdate(lpObjectGroup);
		}

		//Check if any need to be removed
		for (const auto &lpObjectGroup : this->mObjectGroups)
		{
			if (OBJECT_TIMEOUT < chrono ::duration_cast<chrono::minutes>(chrono::steady_clock::now() - lpObjectGroup->GetUpdateTimestamp()).count())
			{
				// TODO: Delete lpObjectGroup
			}
		}

		this->PrintAllObjects();
		this_thread::sleep_for(chrono::seconds(2));
	}
	cout << "ObjectGroupManagerClass thread is stopping." << endl;
}