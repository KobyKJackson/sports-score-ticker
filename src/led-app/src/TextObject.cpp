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
TextObjectClass::TextObjectClass(vector<uint8_t> aLocation, string aValue) : ObjectTypeClass(),
																			 BaseObjectClass(BASE_OBJECT_TYPE::TEXT, aLocation, aValue)
{
	this->mFont = new Font();
	if (this->GetHeight() == 16)
	{
		const char *lpFontFile = "../libs/rpi-rgb-led-matrix/fonts/6x12.bdf";
		if (!this->mFont->LoadFont(lpFontFile))
		{
			fprintf(stderr, "Couldn't load font '%s'\n", lpFontFile);
		}
	}
	else if (this->GetHeight() == 32)
	{
		const char *lpFontFile = "../libs/rpi-rgb-led-matrix/fonts/9x18.bdf";
		if (!this->mFont->LoadFont(lpFontFile))
		{
			fprintf(stderr, "Couldn't load font '%s'\n", lpFontFile);
		}
	}

	this->calculateLength();
}

/* Class Destructor ----------------------------------------------------------*/
TextObjectClass::~TextObjectClass()
{
	delete this->mFont;
}

/* Public Virtual Class Methods ----------------------------------------------*/
OBJECT_TYPE TextObjectClass::GetObjectType() const
{
	return OBJECT_TYPE::TEXT;
};

TextObjectClass *TextObjectClass::clone() const
{
	TextObjectClass *lpTextObjectClass = new TextObjectClass(*this);

	Font *lpFont = new Font();
	if (((BaseObjectClass *)this)->GetHeight() == 16)
	{
		const char *lpFontFile = "../libs/rpi-rgb-led-matrix/fonts/6x12.bdf";
		if (!lpFont->LoadFont(lpFontFile))
		{
			fprintf(stderr, "Couldn't load font '%s'\n", lpFontFile);
		}
	}
	else if (((BaseObjectClass *)this)->GetHeight() == 32)
	{
		const char *lpFontFile = "../libs/rpi-rgb-led-matrix/fonts/9x18.bdf";
		if (!lpFont->LoadFont(lpFontFile))
		{
			fprintf(stderr, "Couldn't load font '%s'\n", lpFontFile);
		}
	}

	lpTextObjectClass->setFont(lpFont);

	return lpTextObjectClass;
}
/* Public Static Class Methods -----------------------------------------------*/

/* Public Class Methods ------------------------------------------------------*/
rgb_matrix::Font *TextObjectClass::GetFont()
{
	return this->mFont;
}
/* Protected Static Class Methods --------------------------------------------*/

/* Protected Class Methods ---------------------------------------------------*/

/* Private Virtual Class Methods ----------------------------------------------*/
void TextObjectClass::calculateLength()
{
	uint32_t lTotalWidth = 0;
	string lValue = this->GetValue();

	for (size_t i = 0; i < lValue.length(); ++i)
	{
		uint8_t lCharWidth = this->mFont->CharacterWidth(static_cast<uint32_t>(lValue[i])); // TODO: font needs to be added to a class
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
void TextObjectClass::setFont(rgb_matrix::Font *aValue)
{
	this->mFont = aValue;
}