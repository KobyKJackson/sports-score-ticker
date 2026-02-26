/*******************************************************************************
** @file       DisplayObject.h
** @class      DisplayObjectClass
** @brief      Unified base class for all displayable objects (text, image, multi)
**
*******************************************************************************/

#ifndef _DISPLAY_OBJECT_H_
#define _DISPLAY_OBJECT_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string>
#include <vector>

/* Exported Constants --------------------------------------------------------*/
constexpr int ROW_SIZE = 16; // 16 pixels per row

/* Exported Types ------------------------------------------------------------*/
enum class OBJECT_TYPE
{
	UNKNOWN = 0,
	TEXT,
	IMAGE,
	MULTI
};

/* Exported Classes ----------------------------------------------------------*/
class DisplayObjectClass
{
public:
	DisplayObjectClass();
	DisplayObjectClass(OBJECT_TYPE aObjectType, std::vector<uint8_t> aLocation, std::string aValue);
	virtual ~DisplayObjectClass();

	virtual OBJECT_TYPE GetObjectType() const = 0;
	virtual DisplayObjectClass *clone() const = 0;

	static OBJECT_TYPE StringTypeToEnumType(const std::string &aValue);

	uint32_t GetLength() const;

	int GetXPosition() const;
	virtual void SetXPosition(int aValue);

	std::string GetValue() const;
	void SetValue(const std::string &aValue);
	uint8_t GetYPosition() const;
	uint8_t GetHeight() const;

protected:
	uint32_t mLength;
	int mXPosition;

private:
	virtual void calculateLength() = 0;

	OBJECT_TYPE mObjectType;
	std::string mValue;
	uint8_t mYPosition;
	uint8_t mHeight;
};

/* Exported Functions --------------------------------------------------------*/

/* Exported Data -------------------------------------------------------------*/

#endif // _DISPLAY_OBJECT_H_
