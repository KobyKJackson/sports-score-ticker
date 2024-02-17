/*******************************************************************************
** @file       ImageUtility.h
** @class      ImageUtilityClass
 * @author     Name Name
 * @version    1.00
 * @date       November 3 2023
**
** @brief
**
*******************************************************************************/

#ifndef _IMAGE_UTILITY_H_
#define _IMAGE_UTILITY_H_

/* Includes ------------------------------------------------------------------*/
#include <curl/curl.h>
#include <png.h>
#include <stdint.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

/* Local Forward Declarations ------------------------------------------------*/

/* Exported Constants --------------------------------------------------------*/

/* Exported Types ------------------------------------------------------------*/
/* Exported Classes ----------------------------------------------------------*/
class ImageUtilityClass
{
public:
	ImageUtilityClass();
	~ImageUtilityClass();

	static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::vector<unsigned char> *buffer);
	static bool DownloadPNGImage(const std::string &url, std::vector<unsigned char> &buffer);
	static void SavePPM(const std::string &filename, png_bytep *row_pointers, int width, int height);

private:
};

/* Exported Functions --------------------------------------------------------*/

/* Exported Data -------------------------------------------------------------*/

#endif // _IMAGE_UTILITY_H_
