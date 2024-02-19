/*******************************************************************************
** @file       DisplayManager.h
** @class      DisplayManagerClass
 * @author     Name Name
 * @version    1.00
 * @date       November 3 2023
**
** @brief
**
*******************************************************************************/

#ifndef _DISPLAY_MANAGER_H_
#define _DISPLAY_MANAGER_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <mutex>
#include <thread>
#include <vector>

#include "ObjectGroup.h"
#include "ObjectGroupManager.h"

/* Local Forward Declarations ------------------------------------------------*/

/* Exported Constants --------------------------------------------------------*/

/* Exported Types ------------------------------------------------------------*/

/* Exported Classes ----------------------------------------------------------*/
class DisplayManagerClass
{
public:
	DisplayManagerClass(ObjectGroupManagerClass *aObjectGroupManager);
	virtual ~DisplayManagerClass();

	std::mutex &GetDataLock();

private:
	void threadFunction();
	ObjectGroupManagerClass *mObjectGroupManager;
	std::vector<ObjectGroupClass> mDisplayObjects;

	uint32_t mObjectIndex;

	std::mutex mDataLock;
	std::thread mThread;
	bool mIsThreadRunning;
};

/* Exported Functions --------------------------------------------------------*/

/* Exported Data -------------------------------------------------------------*/

#endif // _DISPLAY_MANAGER_H_
