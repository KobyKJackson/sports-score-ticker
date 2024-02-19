/*******************************************************************************
** @file       ObjectGroup.h
** @class      ObjectGroupClass
* @author     Name Name
* @version    1.00
* @date       November 3 2023
**
** @brief
**
*******************************************************************************/

#ifndef _OBJECT_GROUP_H_
#define _OBJECT_GROUP_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string>
#include <vector>
#include <chrono>

/* Local Forward Declarations ------------------------------------------------*/
class ObjectTypeClass;
/* Exported Constants --------------------------------------------------------*/

/* Exported Types ------------------------------------------------------------*/

/* Exported Classes ----------------------------------------------------------*/
class ObjectGroupClass
{
public:
	ObjectGroupClass(std::string aID);
	ObjectGroupClass(const ObjectGroupClass &aValue);
	virtual ~ObjectGroupClass();

	std::string GetID() const;
	int GetLength() const;

	int GetXPosition() const;
	void SetXPosition(int aValue);
	void IncrementXPosition();

	void AddObject(ObjectTypeClass *&aObject);
	void RemoveAllObjects();
	ObjectTypeClass *GetByIndex(size_t aIndex);
	size_t GetNumberOfObjects();
	std::vector<ObjectTypeClass *> GetObjects();

	std::chrono::time_point<std::chrono::steady_clock> GetUpdateTimestamp();

	bool
	operator==(const ObjectGroupClass &other) const
	{
		return this->mID == other.mID;
	}

private:
	void calculateLength();
	void updateChilderenXPosition();

	std::vector<ObjectTypeClass *> mObjects;
	std::string mID;
	std::chrono::time_point<std::chrono::steady_clock> mUpdateTimestamp;
	int mLength;
	int mXPosition;
	uint8_t mYPosition; // This will only ever have a max of ROW_SIZE * 3
};

/* Exported Functions --------------------------------------------------------*/

/* Exported Data -------------------------------------------------------------*/

#endif // _OBJECT_GROUP_H_
