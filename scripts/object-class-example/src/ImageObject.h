/*******************************************************************************
** @file       ImageObject.h
** @class      ImageObjectClass
 * @author     Name Name
 * @version    1.00
 * @date       November 3 2023
**
** @brief
**
*******************************************************************************/

#ifndef _IMAGE_OBJECT_H_
#define _IMAGE_OBJECT_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#include "BaseObject.h"
#include "ObjectType.h"

/* Local Forward Declarations ------------------------------------------------*/

/* Exported Constants --------------------------------------------------------*/

/* Exported Types ------------------------------------------------------------*/
/* Exported Classes ----------------------------------------------------------*/
class ImageObjectClass : public BaseObjectClass
  , public ObjectTypeClass
{
public:
	ImageObjectClass(std::vector<uint8_t> aLocation, std::string aValue);
	virtual ~ImageObjectClass();

	virtual OBJECT_TYPE GetObjectType() const;

private:
};

/* Exported Functions --------------------------------------------------------*/

/* Exported Data -------------------------------------------------------------*/

#endif // _IMAGE_OBJECT_H_
