/*******************************************************************************
** @file       ImageObject.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "ImageObject.h"

#include "ImageUtility.h"

/* Exported Data -------------------------------------------------------------*/

/* Static Class Member Initialization ----------------------------------------*/

/* Class Constructors --------------------------------------------------------*/
ImageObjectClass::ImageObjectClass(std::vector<uint8_t> aLocation, std::string aValue) :
  ObjectTypeClass(), BaseObjectClass(BASE_OBJECT_TYPE::IMAGE, aLocation, aValue)
{
	std::string outputFolder = "images";
	std::vector<unsigned char> buffer;

	if (!ImageUtilityClass::DownloadPNGImage(aValue, buffer))
	{
		std::cerr << "Failed to download image." << std::endl;
	}

	std::string filename = aValue.substr(aValue.find_last_of('/') + 1);
	std::string outputFilename = filename.substr(0, filename.find_last_of('.')) + ".ppm";
	std::string outputFilePath = outputFolder + "/" + outputFilename;

	png_image image;
	memset(&image, 0, (sizeof image));
	image.version = PNG_IMAGE_VERSION;

	if (png_image_begin_read_from_memory(&image, buffer.data(), buffer.size()))
	{
		image.format = PNG_FORMAT_RGB;
		png_bytep buffer = new png_byte[PNG_IMAGE_SIZE(image)];
		png_bytep row_pointers[image.height];

		for (size_t y = 0; y < image.height; y++)
		{
			row_pointers[y] = &buffer[y * image.width * 3];
		}

		if (png_image_finish_read(&image, nullptr, buffer, 0, nullptr))
		{
			ImageUtilityClass::SavePPM(outputFilePath, row_pointers, image.width, image.height);
		}
		else
		{
			std::cerr << "Failed to read PNG image." << std::endl;
		}

		delete[] buffer;
	}
	else
	{
		std::cerr << "Failed to start reading PNG image." << std::endl;
	}

	png_image_free(&image);
}

/* Class Destructor ----------------------------------------------------------*/
ImageObjectClass::~ImageObjectClass()
{
}

/* Public Virtual Class Methods ----------------------------------------------*/
OBJECT_TYPE ImageObjectClass::GetObjectType() const
{
	return OBJECT_TYPE::IMAGE;
};
/* Public Static Class Methods -----------------------------------------------*/

/* Public Class Methods ------------------------------------------------------*/

/* Protected Static Class Methods --------------------------------------------*/

/* Protected Class Methods ---------------------------------------------------*/

/* Private Static Class Methods ----------------------------------------------*/

/* Private Class Methods -----------------------------------------------------*/
