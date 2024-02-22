/*******************************************************************************
** @file       ImageObject.h
** @class      ImageObjectClass
* @author     Name Name
* @version    1.00
* @date       November 3 2023
**
** @brief
**
*******************************************************************************/

#ifndef _IMAGE_OBJECT_H_
#define _IMAGE_OBJECT_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string>

#include "BaseObject.h"
#include "ObjectType.h"
#include <Magick++.h>
#include <magick/image.h>

/* Local Forward Declarations ------------------------------------------------*/

/* Exported Constants --------------------------------------------------------*/

/* Exported Types ------------------------------------------------------------*/
/* Exported Classes ----------------------------------------------------------*/
class ImageObjectClass : public BaseObjectClass, public ObjectTypeClass
{
public:
	ImageObjectClass(std::vector<uint8_t> aLocation, std::string aValue);
	virtual ~ImageObjectClass();

	virtual OBJECT_TYPE GetObjectType() const override;
	ImageObjectClass *clone() const override;

	Magick::Image GetImage() const;

private:
	virtual void calculateLength() override;
	static std::string getFilePathFromUrl(const std::string &aURL, const std::string &aPath);
	static size_t writeData(void *ptr, size_t size, size_t nmemb, FILE *stream);
	bool downloadImage(const std::string &aUrl, const std::string &aFilename);
	void createImage();

	Magick::Image mImage;
};

/* Exported Functions --------------------------------------------------------*/

/* Exported Data -------------------------------------------------------------*/

#endif // _IMAGE_OBJECT_H_
