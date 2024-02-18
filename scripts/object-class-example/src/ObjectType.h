/*******************************************************************************
** @file       ObjectType.h
** @class      ObjectTypeClass
 * @author     Name Name
 * @version    1.00
 * @date       November 3 2023
**
** @brief
**
*******************************************************************************/

#ifndef _OBJECT_TYPE_H_
#define _OBJECT_TYPE_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string>
#include <vector>

/* Local Forward Declarations ------------------------------------------------*/

/* Exported Constants --------------------------------------------------------*/

/* Exported Types ------------------------------------------------------------*/
enum class OBJECT_TYPE
{
	UNKNOWN = 0,
	TEXT,
	IMAGE,
	MULTI
};

/* Exported Classes ----------------------------------------------------------*/
class ObjectTypeClass
{
public:
	ObjectTypeClass();
	virtual ~ObjectTypeClass();

	virtual OBJECT_TYPE GetObjectType() const = 0;

	static OBJECT_TYPE StringTypeToEnumType(const std::string &aValue);

	uint32_t GetLength() const;

protected:
	uint32_t mLength;

private:
	virtual void calculateLength() = 0;

	uint32_t mXPosition;
};

/* Exported Functions --------------------------------------------------------*/

/* Exported Data -------------------------------------------------------------*/

#endif // _OBJECT_TYPE_H_
