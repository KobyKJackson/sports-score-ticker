/*******************************************************************************
** @file       TextObject.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "TextObject.h"

#include <iostream>

using namespace std;
/* Exported Data -------------------------------------------------------------*/

/* Static Class Member Initialization ----------------------------------------*/

/* Class Constructors --------------------------------------------------------*/
TextObjectClass::TextObjectClass(vector<uint8_t> aLocation, string aValue) :
  ObjectTypeClass(), BaseObjectClass(BASE_OBJECT_TYPE::TEXT, aLocation, aValue)
{
	this->calculateLength();
}

/* Class Destructor ----------------------------------------------------------*/
TextObjectClass::~TextObjectClass()
{
}

/* Public Virtual Class Methods ----------------------------------------------*/
OBJECT_TYPE TextObjectClass::GetObjectType() const
{
	return OBJECT_TYPE::TEXT;
};

TextObjectClass *TextObjectClass::clone() const
{
	return new TextObjectClass(*this);
}
/* Public Static Class Methods -----------------------------------------------*/

/* Public Class Methods ------------------------------------------------------*/

/* Protected Static Class Methods --------------------------------------------*/

/* Protected Class Methods ---------------------------------------------------*/

/* Private Virtual Class Methods ----------------------------------------------*/
void TextObjectClass::calculateLength()
{
	uint32_t lTotalWidth = 0;
	string lValue = this->GetValue();

	for (size_t i = 0; i < lValue.length(); ++i)
	{
		uint8_t lCharWidth = 1; //font.CharacterWidth(static_cast<uint32_t>(lValue[i])); //TODO: font needs to be added to a class
		if (lCharWidth > 0)
		{
			lTotalWidth += lCharWidth;
		}
		else
		{
			cout << "BAD CHARACTER IN STRING: " << lValue[i] << endl;
		}
	}
	this->mLength = lTotalWidth;
}
/* Private Static Class Methods ----------------------------------------------*/

/* Private Class Methods -----------------------------------------------------*/
