/*******************************************************************************
** @file       ImageUtility.cpp
** @author     Name Name
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "ImageUtility.h"

/* Exported Data -------------------------------------------------------------*/

/* Static Class Member Initialization ----------------------------------------*/

/* Class Constructors --------------------------------------------------------*/
ImageUtilityClass::ImageUtilityClass()
{
}

/* Class Destructor ----------------------------------------------------------*/
ImageUtilityClass::~ImageUtilityClass()
{
}

size_t ImageUtilityClass::WriteCallback(void *contents, size_t size, size_t nmemb, std::vector<unsigned char> *buffer)
{
	size_t totalSize = size * nmemb;
	buffer->insert(buffer->end(), (unsigned char *)contents, (unsigned char *)contents + totalSize);
	return totalSize;
}

bool ImageUtilityClass::DownloadPNGImage(const std::string &url, std::vector<unsigned char> &buffer)
{
	CURL *curl = curl_easy_init();
	if (!curl)
		return false;

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ImageUtilityClass::WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	return res == CURLE_OK;
}

void ImageUtilityClass::SavePPM(const std::string &filename, png_bytep *row_pointers, int width, int height)
{
	std::ofstream ofs(filename, std::ios::binary);
	ofs << "P6\n"
	    << width << " " << height << "\n255\n";
	for (int y = 0; y < height; y++)
	{
		ofs.write(reinterpret_cast<char *>(row_pointers[y]), width * 3);
	}
}

/* Public Virtual Class Methods ----------------------------------------------*/

/* Public Static Class Methods -----------------------------------------------*/

/* Public Class Methods ------------------------------------------------------*/

/* Protected Static Class Methods --------------------------------------------*/

/* Protected Class Methods ---------------------------------------------------*/

/* Private Static Class Methods ----------------------------------------------*/

/* Private Class Methods -----------------------------------------------------*/
