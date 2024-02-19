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

/* Exported Constants --------------------------------------------------------*/

/* Exported Types ------------------------------------------------------------*/
/* Exported Classes ----------------------------------------------------------*/
class MultiObjectClass : public ObjectTypeClass
{
public:
	MultiObjectClass();
	virtual ~MultiObjectClass();

	virtual OBJECT_TYPE GetObjectType() const override;
	MultiObjectClass *clone() const override;

	void AddObject(ObjectTypeClass *&aObject);
	ObjectTypeClass *GetByIndex(size_t aIndex);
	size_t GetNumberOfObjects();

private:
	virtual void calculateLength() override;

	std::vector<ObjectTypeClass *> mObjects;
};

/* Exported Functions --------------------------------------------------------*/

/* Exported Data -------------------------------------------------------------*/

#endif // _MULTI_OBJECT_H_
