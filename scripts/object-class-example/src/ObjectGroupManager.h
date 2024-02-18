/*******************************************************************************
** @file       ObjectGroupManager.h
** @class      ObjectGroupManagerClass
 * @author     Name Name
 * @version    1.00
 * @date       November 3 2023
**
** @brief
**
*******************************************************************************/

#ifndef _OBJECT_GROUP_MANAGER_H_
#define _OBJECT_GROUP_MANAGER_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string>
#include <thread>
#include <vector>

/* Local Forward Declarations ------------------------------------------------*/
class ObjectGroupClass;

/* Exported Constants --------------------------------------------------------*/

/* Exported Types ------------------------------------------------------------*/

/* Exported Classes ----------------------------------------------------------*/
class ObjectGroupManagerClass
{
public:
	ObjectGroupManagerClass();
	virtual ~ObjectGroupManagerClass();

	void AddOrUpdate(ObjectGroupClass *&aValue);
	bool RemoveByID(std::string aID);
	ObjectGroupClass *GetByID(std::string aID);
	ObjectGroupClass *GetByIndex(size_t aIndex);
	size_t GetNumberOfObjectGroups();
	void PrintAllObjects();

private:
	void threadFunction();
	std::vector<ObjectGroupClass *> mObjectGroups;
	std::thread mThread;
	bool mIsThreadRunning;
};

/* Exported Functions --------------------------------------------------------*/

/* Exported Data -------------------------------------------------------------*/

#endif // _OBJECT_GROUP_MANAGER_H_
