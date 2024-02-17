#include <chrono>
#include <fstream>
#include <iostream>
#include <stack>
#include <string>
#include <thread>
#include <vector>

#include "ObjectGroup.h"
#include "ObjectGroupManager.h"

#include "BaseObject.h"
#include "ImageObject.h"
#include "MultiObject.h"
#include "ObjectType.h"
#include "TextObject.h"

#include "json.hpp"

using json = nlohmann::json;

void PrintAllObjects(ObjectGroupManagerClass aObjectGroupManager)
{
	for (uint8_t i = 0; i < aObjectGroupManager.GetNumberOfObjectGroups(); i++)
	{
		ObjectGroupClass *lpObjectGroup = aObjectGroupManager.GetByIndex(i);

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

int main()
{
	using namespace std::chrono;
	auto start = high_resolution_clock::now();

	ObjectGroupManagerClass lObjectGroupManager;

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
		ObjectGroupClass *lpObjectGroup = lObjectGroupManager.GetByID(lID);

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
		lObjectGroupManager.AddOrUpdate(lpObjectGroup);
	}

	PrintAllObjects(lObjectGroupManager);

	//while (true)
	{
		std::this_thread::sleep_until(start + 1000ms);
		start = high_resolution_clock::now();
	}

	return 0;
}