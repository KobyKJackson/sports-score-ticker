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

/* Local Forward Declarations ------------------------------------------------*/
class ObjectTypeClass;
/* Exported Constants --------------------------------------------------------*/

/* Exported Types ------------------------------------------------------------*/

/* Exported Classes ----------------------------------------------------------*/
class ObjectGroupClass
{
public:
	ObjectGroupClass(std::string aID);
	virtual ~ObjectGroupClass();

	std::string GetID() const;
	void AddObject(ObjectTypeClass *&aObject);
	ObjectTypeClass *GetByIndex(size_t aIndex);
	size_t GetNumberOfObjects();

	bool operator==(const ObjectGroupClass &other) const
	{
		return this->mID == other.mID;
	}

private:
	std::vector<ObjectTypeClass *> mObjects;

	std::string mID;

	uint32_t mXPosition;
	uint8_t mYPosition; //This will only ever have a max of ROW_SIZE * 3
};

/* Exported Functions --------------------------------------------------------*/

/* Exported Data -------------------------------------------------------------*/

#endif // _OBJECT_GROUP_H_
