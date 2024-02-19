/*******************************************************************************
** @file       DisplayManager.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "DisplayManager.h"

#include <iostream>
#include "ObjectGroupManager.h"

using namespace std;

/* Exported Data -------------------------------------------------------------*/

/* Static Class Member Initialization ----------------------------------------*/

/* Class Constructors --------------------------------------------------------*/
DisplayManagerClass::DisplayManagerClass(ObjectGroupManagerClass *aObjectGroupManager, uint32_t aDisplayWidth) :
  mObjectGroupManager(aObjectGroupManager),
  mDisplayWidth(aDisplayWidth),
  mObjectIndex(0),
  mIsThreadRunning(true)
{
	this->mThread = thread(&DisplayManagerClass::threadFunction, this);
}

/* Class Destructor ----------------------------------------------------------*/
DisplayManagerClass::~DisplayManagerClass()
{
	lock_guard<mutex> lLock(this->mDataLock);
	this->mIsThreadRunning = false;

	if (this->mThread.joinable())
	{
		this->mThread.join();
	}
}

/* Public Static Class Methods -----------------------------------------------*/

/* Public Class Methods ------------------------------------------------------*/
mutex &DisplayManagerClass::GetDataLock()
{
	return this->mDataLock;
}

/* Protected Static Class Methods --------------------------------------------*/

/* Protected Class Methods ---------------------------------------------------*/

/* Private Static Class Methods ----------------------------------------------*/

/* Private Class Methods -----------------------------------------------------*/
void DisplayManagerClass::threadFunction()
{
	cout << "DisplayManagerClass thread is running." << endl;
	while (this->mIsThreadRunning)
	{
		if ((this->mDisplayObjects.size() == 0) ||
		    (this->mDisplayObjects[this->mDisplayObjects.size() - 1].GetXPosition() + this->mDisplayObjects[this->mDisplayObjects.size() - 1].GetLength() == this->mDisplayWidth)) //Should we add an object?
		{
			unique_lock<mutex> lLock(this->mDataLock);
			ObjectGroupClass *lpObjectGroup = this->mObjectGroupManager->GetByIndex(this->mObjectIndex);
			if (lpObjectGroup != nullptr)
			{
				uint32_t lTotalWidth = this->mDisplayWidth;

				if (this->mDisplayObjects.size() != 0)
				{
					for (const auto &lpObject : this->mDisplayObjects)
					{
						lTotalWidth += lpObject.GetLength();
					}
					lTotalWidth += mDisplayObjects[0].GetXPosition();
				}

				lpObjectGroup->SetXPosition(lTotalWidth);
				this->mDisplayObjects.push_back(*lpObjectGroup);
				this->mObjectIndex++;
			}
		}

		if ((this->mDisplayObjects.size() != 0) &&
		    (this->mDisplayObjects[0].GetXPosition() < (-1 * this->mDisplayObjects[0].GetLength()))) //Should we delete an object?
		{
			unique_lock<mutex> lLock(this->mDataLock);
			if (!this->mDisplayObjects.empty())
			{
				this->mDisplayObjects.erase(this->mDisplayObjects.begin());
			}
		}

		if (this->mObjectIndex >= this->mObjectGroupManager->GetNumberOfObjectGroups())
		{
			this->mObjectIndex = 0;
		}

		this_thread::sleep_for(chrono::seconds(1));
	}
	cout << "DisplayManagerClass thread is stopping." << endl;
}