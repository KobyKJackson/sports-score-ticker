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

#include "DisplayObject.h"
#include "led-matrix.h"

/* Exported Classes ----------------------------------------------------------*/
class TextObjectClass : public DisplayObjectClass
{
public:
	TextObjectClass(std::vector<uint8_t> aLocation, std::string aValue);
	virtual ~TextObjectClass();

	virtual OBJECT_TYPE GetObjectType() const override;
	TextObjectClass *clone() const override;

	rgb_matrix::Font *GetFont();

private:
	virtual void calculateLength() override;
	void setFont(rgb_matrix::Font *aValue);

	rgb_matrix::Font *mFont;
	std::string mColor;
};

/* Exported Functions --------------------------------------------------------*/

/* Exported Data -------------------------------------------------------------*/

#endif // _TEXT_OBJECT_H_
