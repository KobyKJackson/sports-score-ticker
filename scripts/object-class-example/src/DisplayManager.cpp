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
DisplayManagerClass::DisplayManagerClass(ObjectGroupManagerClass *aObjectGroupManager) :
  mObjectGroupManager(aObjectGroupManager),
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
		if (true) //Should we add an object?
		{
			unique_lock<mutex> lLock(this->mDataLock);
			ObjectGroupClass *lpObjectGroup = this->mObjectGroupManager->GetByIndex(this->mObjectIndex);
			if (lpObjectGroup != nullptr)
			{
				this->mDisplayObjects.push_back(*lpObjectGroup);
				this->mObjectIndex++;
			}
		}

		if (true) //Should we delete an object?
		{
			unique_lock<mutex> lLock(this->mDataLock);
			if (!this->mDisplayObjects.empty())
			{
				this->mDisplayObjects.erase(this->mDisplayObjects.begin());
			}
		}

		this_thread::sleep_for(chrono::seconds(1));
	}
	cout << "DisplayManagerClass thread is stopping." << endl;
}