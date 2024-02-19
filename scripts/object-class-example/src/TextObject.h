/*******************************************************************************
** @file       TextObject.h
** @class      TextObjectClass
 * @author     Name Name
 * @version    1.00
 * @date       November 3 2023
**
** @brief
**
*******************************************************************************/

#ifndef _TEXT_OBJECT_H_
#define _TEXT_OBJECT_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#include "BaseObject.h"
#include "ObjectType.h"

/* Local Forward Declarations ------------------------------------------------*/

/* Exported Constants --------------------------------------------------------*/

/* Exported Types ------------------------------------------------------------*/
/* Exported Classes ----------------------------------------------------------*/
class TextObjectClass : public BaseObjectClass
  , public ObjectTypeClass
{
public:
	TextObjectClass(std::vector<uint8_t> aLocation, std::string aValue);
	virtual ~TextObjectClass();

	virtual OBJECT_TYPE GetObjectType() const override;
	TextObjectClass *clone() const override;

private:
	virtual void calculateLength() override;

	std::string mColor;
};

/* Exported Functions --------------------------------------------------------*/

/* Exported Data -------------------------------------------------------------*/

#endif // _TEXT_OBJECT_H_
