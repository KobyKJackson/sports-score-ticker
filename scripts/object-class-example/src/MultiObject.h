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

#include "ObjectType.h"

/* Local Forward Declarations ------------------------------------------------*/
class BaseObjectClass;

/* Exported Constants --------------------------------------------------------*/

/* Exported Types ------------------------------------------------------------*/
/* Exported Classes ----------------------------------------------------------*/
class MultiObjectClass : public ObjectTypeClass
{
public:
	MultiObjectClass();
	virtual ~MultiObjectClass();

	virtual OBJECT_TYPE GetObjectType() const;

	void AddObject(BaseObjectClass *&aObject);
	BaseObjectClass *GetByIndex(size_t aIndex);
	size_t GetNumberOfObjects();

private:
	std::vector<BaseObjectClass *> mObjects;
};

/* Exported Functions --------------------------------------------------------*/

/* Exported Data -------------------------------------------------------------*/

#endif // _MULTI_OBJECT_H_
