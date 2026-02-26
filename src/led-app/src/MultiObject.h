/*******************************************************************************
** @file       MultiObject.h
** @class      MultiObjectClass
* @author     Name Name
* @version    1.00
* @date       November 3 2023
**
** @brief
**
*******************************************************************************/

#ifndef _MULTI_OBJECT_H_
#define _MULTI_OBJECT_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#include "DisplayObject.h"

/* Exported Classes ----------------------------------------------------------*/
class MultiObjectClass : public DisplayObjectClass
{
public:
	MultiObjectClass();
	virtual ~MultiObjectClass();

	virtual OBJECT_TYPE GetObjectType() const override;
	MultiObjectClass *clone() const override;

	void AddObject(DisplayObjectClass *aObject);
	void RemoveAllObjects();
	DisplayObjectClass *GetByIndex(size_t aIndex);
	size_t GetNumberOfObjects();
	void SetXPosition(int aValue);

private:
	virtual void calculateLength() override;

	std::vector<DisplayObjectClass *> mObjects;
};

/* Exported Functions --------------------------------------------------------*/

/* Exported Data -------------------------------------------------------------*/

#endif // _MULTI_OBJECT_H_
