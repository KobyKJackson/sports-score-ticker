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
					std::cout << "data: " << lpImageObject->GetValue() << std::endl;
				}
				break;

				case OBJECT_TYPE::TEXT:
				{
					TextObjectClass *lpTextObject = static_cast<TextObjectClass *>(lpObject);
					std::cout << "data: " << lpTextObject->GetValue() << std::endl;
				}
				break;

				case OBJECT_TYPE::MULTI:
				{
					MultiObjectClass *lpMultiObject = static_cast<MultiObjectClass *>(lpObject);
					for (uint8_t k = 0; k < lpMultiObject->GetNumberOfObjects(); k++)
					{
						BaseObjectClass *lpBaseObject = lpMultiObject->GetByIndex(k);
						switch (lpBaseObject->GetBaseObjectType())
						{
							case BASE_OBJECT_TYPE::IMAGE:
							{
								ImageObjectClass *lpImageObject = static_cast<ImageObjectClass *>(lpBaseObject);
								std::cout << "data: " << lpImageObject->GetValue() << std::endl;
							}
							break;

							case BASE_OBJECT_TYPE::TEXT:
							{
								TextObjectClass *lpTextObject = static_cast<TextObjectClass *>(lpBaseObject);
								std::cout << "data: " << lpTextObject->GetValue() << std::endl;
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
			std::ifstream f("../src/example.json");
			lJSONData = json::parse(f);
			std::cout << "Example element: " << lJSONData << std::endl;
		}
		catch (json::parse_error &e)
		{
			std::cerr << "JSON parse error: " << e.what() << std::endl;
		}

		//Update Objects
		for (const auto &lGame : lJSONData["games"])
		{
			std::string lID = lGame["id"];
			ObjectGroupClass *lpObjectGroup = this->GetByID(lID);

			if (lpObjectGroup != nullptr)
			{
				//Update
			}
			else
			{
				lpObjectGroup = new ObjectGroupClass(lID);

				for (const auto &lElement : lGame["data"])
				{
					OBJECT_TYPE lType = ObjectTypeClass::StringTypeToEnumType(lElement["type"]);
					ObjectTypeClass *lpObject = nullptr;
					switch (lType)
					{
						case OBJECT_TYPE::IMAGE:
						{
							std::vector<uint8_t> lLocation = lElement["location"];
							std::string lData = lElement["data"];
							lpObject = new ImageObjectClass(lLocation, lData);
						}
						break;

						case OBJECT_TYPE::TEXT:
						{
							std::vector<uint8_t> lLocation = lElement["location"];
							std::string lData = lElement["data"];
							lpObject = new TextObjectClass(lLocation, lData);
						}
						break;

						case OBJECT_TYPE::MULTI:
						{
							lpObject = new MultiObjectClass();

							for (const auto &lBaseElement : lElement["data"])
							{
								BASE_OBJECT_TYPE lBaseType = BaseObjectClass::StringTypeToEnumType(lBaseElement["type"]);
								BaseObjectClass *lpBaseObject = nullptr;
								switch (lBaseType)
								{
									case BASE_OBJECT_TYPE::IMAGE:
									{
										std::vector<uint8_t> lLocation = lBaseElement["location"];
										std::string lData = lBaseElement["data"];
										lpBaseObject = new ImageObjectClass(lLocation, lData);
									}
									break;

									case BASE_OBJECT_TYPE::TEXT:
									{
										std::vector<uint8_t> lLocation = lBaseElement["location"];
										std::string lData = lBaseElement["data"];
										lpBaseObject = new TextObjectClass(lLocation, lData);
									}
									break;

									default:
										break;
								}
								(static_cast<MultiObjectClass *>(lpObject))->AddObject(lpBaseObject);
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
			}
			this->AddOrUpdate(lpObjectGroup);
		}

		this->PrintAllObjects();
		this_thread::sleep_for(chrono::seconds(10));
	}
	cout << "ObjectGroupManagerClass thread is stopping." << endl;
}