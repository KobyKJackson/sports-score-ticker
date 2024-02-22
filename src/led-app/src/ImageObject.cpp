/*******************************************************************************
** @file       ImageObject.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "ImageObject.h"

#include <filesystem>
#include <curl/curl.h>
#include <fstream>
#include <iostream>

using namespace std;
namespace fs = filesystem;

/* Exported Data -------------------------------------------------------------*/

/* Static Class Member Initialization ----------------------------------------*/

/* Class Constructors --------------------------------------------------------*/
ImageObjectClass::ImageObjectClass(vector<uint8_t> aLocation, string aValue) : ObjectTypeClass(), BaseObjectClass(BASE_OBJECT_TYPE::IMAGE, aLocation, aValue)
{
	string lFileName = this->getFilePathFromUrl(this->GetValue(), "../data/images");

	if (fs::exists(fs::path(lFileName)))
	{
		cout << "Image file already exists for: " << aValue << endl;
		this->SetValue(lFileName);
		this->createImage();
	}
	else
	{
		cout << "File does not exist, proceed with downloading and conversion." << endl;

		if (this->downloadImage(aValue, lFileName))
		{
			cout << "Download successful" << endl;
			this->SetValue(lFileName);
			this->createImage();
		}
		else
		{
			cout << "Download failed" << endl;
		}
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

ImageObjectClass *ImageObjectClass::clone() const
{
	return new ImageObjectClass(*this);
}
/* Public Static Class Methods -----------------------------------------------*/

/* Public Class Methods ------------------------------------------------------*/
Magick::Image ImageObjectClass::GetImage() const
{
	return this->mImage;
};
/* Protected Static Class Methods --------------------------------------------*/

/* Protected Class Methods ---------------------------------------------------*/

/* Private Static Class Methods ----------------------------------------------*/
string ImageObjectClass::getFilePathFromUrl(const string &aURL, const string &aPath)
{
	string lFileName = aURL.substr(aURL.find_last_of('/') + 1);
	string lOutputFilename = lFileName.substr(0, lFileName.find_last_of('.')) + ".png";

	return aPath + "/" + lOutputFilename;
}

size_t ImageObjectClass::writeData(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}

/* Private Class Methods -----------------------------------------------------*/
void ImageObjectClass::calculateLength()
{
	this->mLength = this->mImage.columns();
}

bool ImageObjectClass::downloadImage(const string &aUrl, const string &aFilename)
{
	CURL *curl;
	FILE *fp;
	CURLcode res;
	curl = curl_easy_init();
	if (curl)
	{
		fp = fopen(aFilename.c_str(), "wb");
		curl_easy_setopt(curl, CURLOPT_URL, aUrl.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ImageObjectClass::writeData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);
		return (res == CURLE_OK);
	}
	return false;
}

void ImageObjectClass::createImage()
{
	this->mImage.read(this->GetValue());

	// Scale image to the desired height while maintaining aspect ratio
	double scale_factor = static_cast<double>(this->GetHeight()) / this->mImage.rows();
	int target_width = static_cast<int>(this->mImage.columns() * scale_factor);
	this->mImage.scale(Magick::Geometry(this->GetHeight(), this->GetHeight()));
}
