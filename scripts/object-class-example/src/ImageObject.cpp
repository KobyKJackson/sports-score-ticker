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
	uint8_t maxHeight = this->GetHeight();

	if (!ImageUtilityClass::DownloadPNGImage(aValue, buffer))
	{
		std::cerr << "Failed to download image." << std::endl;
	}

	// Convert the downloaded buffer into an OpenCV Mat
	cv::Mat image = cv::imdecode(cv::Mat(buffer), cv::IMREAD_COLOR);
	if (image.empty())
	{
		std::cerr << "Failed to decode image." << std::endl;
	}

	// Resize the image to the specified maxHeight while maintaining aspect ratio
	double aspectRatio = image.cols / static_cast<double>(image.rows);
	int newWidth = static_cast<int>(aspectRatio * maxHeight);
	cv::Mat resizedImage;
	cv::resize(image, resizedImage, cv::Size(newWidth, maxHeight));

	// Extract filename from URL and change extension to .ppm
	std::string filename = aValue.substr(aValue.find_last_of('/') + 1);
	std::string baseFilename = filename.substr(0, filename.find_last_of('.'));
	std::string outputFilename = outputFolder + "/" + baseFilename + ".ppm";

	// Save the resized image as PPM
	ImageUtilityClass::SavePPM(outputFilename, resizedImage);
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
