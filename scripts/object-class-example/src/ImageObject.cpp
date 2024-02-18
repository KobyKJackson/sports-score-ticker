/*******************************************************************************
** @file       ImageObject.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "ImageObject.h"

#include <filesystem> // Include the filesystem library
#include "ImageUtility.h"

using namespace std;
namespace fs = filesystem;

/* Exported Data -------------------------------------------------------------*/

/* Static Class Member Initialization ----------------------------------------*/

/* Class Constructors --------------------------------------------------------*/
ImageObjectClass::ImageObjectClass(vector<uint8_t> aLocation, string aValue) :
  ObjectTypeClass(), BaseObjectClass(BASE_OBJECT_TYPE::IMAGE, aLocation, aValue)
{
	string lFileName = this->getFilePathFromUrl(this->GetValue(), "../data/images");

	if (fs::exists(fs::path(lFileName)))
	{
		cout << "Image file already exists for: " << aValue << endl;
		this->SetValue(lFileName);
	}
	else
	{
		cout << "File does not exist, proceed with downloading and conversion." << endl;
		//Download and convert
	}
	this->calculateLength();
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
string ImageObjectClass::getFilePathFromUrl(const string &aURL, const string &aPath)
{
	string lFileName = aURL.substr(aURL.find_last_of('/') + 1);
	string lOutputFilename = lFileName.substr(0, lFileName.find_last_of('.')) + ".ppm";

	return aPath + "/" + lOutputFilename;
}

/* Private Class Methods -----------------------------------------------------*/
void ImageObjectClass::calculateLength()
{
	string lValue = this->GetValue();

	ifstream lFile(lValue, ios::binary);
	if (!lFile.is_open())
	{
		cerr << "Failed to open file: " << lValue << endl;
		return;
	}

	string lLine;
	getline(lFile, lLine); // Read the magic number line
	if (lLine != "P6")
	{
		cerr << "Unsupported PPM format or incorrect file: " << lLine << endl;
		return;
	}

	while (lFile.peek() == '#')
	{
		getline(lFile, lLine);
	}

	uint32_t lWidth = 0, lHeight = 0;
	lFile >> lWidth >> lHeight;
	this->mLength = lWidth;
}

void ImageObjectClass::DownloadAndConvertFile()
{
	ImageUtilityClass *lpImageUtility = new ImageUtilityClass();
	string outputFolder = "images";
	vector<unsigned char> buffer;
	uint8_t maxHeight = this->GetHeight();

	string lValue = this->GetValue();

	if (!lpImageUtility->DownloadPNGImage(lValue, buffer))
	{
		cerr << "Failed to download image." << endl;
	}

	string filename = lValue.substr(lValue.find_last_of('/') + 1);
	string outputFilename = filename.substr(0, filename.find_last_of('.')) + ".ppm";
	string outputFilePath = outputFolder + "/" + outputFilename;

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
			int resizedWidth = image.width, resizedHeight = image.height;
			if (image.height > maxHeight)
			{
				resizedHeight = maxHeight;
				resizedWidth = (image.width * maxHeight) / image.height;
				png_bytep *resizedPixels = nullptr;
				lpImageUtility->ResizeImageSimpleNearestNeighbor(row_pointers, image.width, image.height, resizedPixels, resizedWidth, resizedHeight);
				lpImageUtility->SavePPM(outputFilePath, resizedPixels, resizedWidth, resizedHeight);
				for (int y = 0; y < resizedHeight; ++y)
				{
					delete[] resizedPixels[y];
				}
				delete[] resizedPixels;
			}
		}
		else
		{
			cerr << "Failed to read PNG image." << endl;
		}

		delete[] buffer;
	}
	else
	{
		cerr << "Failed to start reading PNG image." << endl;
	}

	png_image_free(&image);
	delete lpImageUtility;
}
