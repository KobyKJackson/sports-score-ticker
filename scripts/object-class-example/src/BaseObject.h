/*******************************************************************************
** @file       BaseObject.h
** @class      BaseObjectClass
 * @author     Name Name
 * @version    1.00
 * @date       November 3 2023
**
** @brief
**
*******************************************************************************/

#ifndef _BASE_OBJECT_H_
#define _BASE_OBJECT_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string>
#include <vector>

/* Local Forward Declarations ------------------------------------------------*/

/* Exported Constants --------------------------------------------------------*/
#define ROW_SIZE 16 //16 pixels

/* Exported Types ------------------------------------------------------------*/
enum class BASE_OBJECT_TYPE
{
	UNKNOWN = 0,
	TEXT,
	IMAGE,
};

/* Exported Classes ----------------------------------------------------------*/
class BaseObjectClass
{
public:
	BaseObjectClass(BASE_OBJECT_TYPE aObjectType, std::vector<uint8_t> aLocation, std::string aValue);
	virtual ~BaseObjectClass();

	static BASE_OBJECT_TYPE StringTypeToEnumType(const std::string &aValue);

	BASE_OBJECT_TYPE GetBaseObjectType() const;
	bool SetValue(std::string aValue);
	std::string GetValue();

private:
	BASE_OBJECT_TYPE mObjectType;

	std::string mValue;

	uint8_t mYPosition; //This will only ever have a max of ROW_SIZE * 3
	uint8_t mHeight;
};

/* Exported Functions --------------------------------------------------------*/

/* Exported Data -------------------------------------------------------------*/

#endif // _BASE_OBJECT_H_
