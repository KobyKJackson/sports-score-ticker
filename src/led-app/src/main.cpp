#include <chrono>
#include <thread>
#include <signal.h>
#include <iostream>

#include "led-matrix.h"
#include "graphics.h"
#include <Magick++.h>
#include "DisplayManager.h"
#include "ObjectGroupManager.h"
#include "ObjectType.h"
#include "TextObject.h"
#include "ImageObject.h"
#include "MultiObject.h"

using namespace rgb_matrix;
using namespace std;

#define PIXEL_WIDTH 64 * 5
#define PIXEL_HEIGHT 32 * 2

volatile bool IsInterruptReceived = false;
static void InterruptHandler(int aSig)
{
	IsInterruptReceived = true;
}

static void AddMicros(struct timespec *aAccumulator, long mMicros)
{
	const long billion = 1000000000;
	const int64_t nanos = (int64_t)mMicros * 1000;
	aAccumulator->tv_sec += nanos / billion;
	aAccumulator->tv_nsec += nanos % billion;
	while (aAccumulator->tv_nsec > billion)
	{
		aAccumulator->tv_nsec -= billion;
		aAccumulator->tv_sec += 1;
	}
}

int main(int argc, char *argv[])
{
	signal(SIGTERM, InterruptHandler);
	signal(SIGINT, InterruptHandler);

	using namespace chrono;
	auto start = high_resolution_clock::now();

	ObjectGroupManagerClass *lpObjectGroupManager = new ObjectGroupManagerClass();
	DisplayManagerClass *lpDisplayManager = new DisplayManagerClass(lpObjectGroupManager, PIXEL_WIDTH);

	// Customize
	int lLetterSpacing = 0;
	float lSpeed = 3.0f;
	int lDelaySpeed_usec = 1000000 / lSpeed / 10; // TODO: Figure this out lFont.CharacterWidth('W');
	Color lColor(255, 255, 255);

	// Setup Matrix
	RGBMatrix::Options lMatrixOptions;
	rgb_matrix::RuntimeOptions lRuntimeOptions;
	Magick::InitializeMagick(*argv);

	lMatrixOptions.rows = 32;
	lMatrixOptions.cols = 64;
	lMatrixOptions.chain_length = 10;
	lMatrixOptions.parallel = 1;
	lMatrixOptions.pixel_mapper_config = "U-mapper;Rotate:180";
	lMatrixOptions.hardware_mapping = "adafruit-hat"; // or "adafruit-hat" depending on your setup
	lMatrixOptions.disable_hardware_pulsing = true;
	lRuntimeOptions.gpio_slowdown = 3;

	RGBMatrix *lMatrix = CreateMatrixFromOptions(lMatrixOptions, lRuntimeOptions);
	if (lMatrix == nullptr)
	{
		cerr << "Could not create matrix object." << endl;
		return 1;
	}

	FrameCanvas *lOffscreenCanvas = lMatrix->CreateFrameCanvas();

	lMatrix->SetPWMBits(1);

	struct timespec lNextFrame = {0, 0};
	uint64_t lFrameCounter = 0;

	while (!IsInterruptReceived)
	{
		++lFrameCounter;
		lOffscreenCanvas->Fill(0, 0, 0);

		for (auto &lpObjectGroup : lpDisplayManager->GetDisplayObjects())
		{
			// Do the thing
			for (auto &lpObject : lpObjectGroup.GetObjects())
			{
				switch (lpObject->GetObjectType())
				{
				case OBJECT_TYPE::IMAGE:
				{
					ImageObjectClass *lpImageObject = static_cast<ImageObjectClass *>(lpObject);

					Magick::Image lImage = lpImageObject->GetImage();

					for (int img_y = 0; img_y < lpImageObject->GetHeight(); ++img_y)
					{
						for (int img_x = 0; img_x < lpImageObject->GetLength(); ++img_x)
						{
							Magick::ColorRGB rgb(lImage.pixelColor(img_x, img_y));
							lOffscreenCanvas->SetPixel(lpImageObject->GetXPosition() + img_x, lpImageObject->GetYPosition() + img_y,
													   rgb.red() * 255, rgb.green() * 255, rgb.blue() * 255);
						}
					}
				}
				break;

				case OBJECT_TYPE::TEXT:
				{
					TextObjectClass *lpTextObject = static_cast<TextObjectClass *>(lpObject);
					rgb_matrix::DrawText(lOffscreenCanvas,
										 *lpTextObject->GetFont(),
										 lpTextObject->GetXPosition(),
										 lpTextObject->GetYPosition() + ((*lpTextObject->GetFont()).baseline()),
										 lColor,
										 NULL,
										 lpTextObject->GetValue().c_str(),
										 lLetterSpacing);
					// cout << lpTextObject->GetValue().c_str() << ": " << lpTextObject->GetXPosition() << ", Length: " << lpObjectGroup.GetLength() << endl;
				}
				break;

				case OBJECT_TYPE::MULTI:
				{
					MultiObjectClass *lpMultiObject = static_cast<MultiObjectClass *>(lpObject);
					for (uint8_t k = 0; k < lpMultiObject->GetNumberOfObjects(); k++)
					{
						ObjectTypeClass *lpObjectType = lpMultiObject->GetByIndex(k);
						switch (lpObjectType->GetObjectType())
						{
						case OBJECT_TYPE::IMAGE:
						{
							// Do nothing
						}
						break;

						case OBJECT_TYPE::TEXT:
						{
							TextObjectClass *lpTextObject = static_cast<TextObjectClass *>(lpObjectType);
							rgb_matrix::DrawText(lOffscreenCanvas,
												 *lpTextObject->GetFont(),
												 lpTextObject->GetXPosition(),
												 lpTextObject->GetYPosition() + ((*lpTextObject->GetFont()).baseline()),
												 lColor,
												 NULL,
												 lpTextObject->GetValue().c_str(),
												 lLetterSpacing);
							// cout << lpTextObject->GetValue().c_str() << ": " << lpTextObject->GetXPosition() << ", Length: " << lpObjectGroup.GetLength() << endl;
						}
						break;

						default:
							break;
						}
					}
				}
				break;

				default:
					break;
				}
			}

			lpObjectGroup.IncrementXPosition();
		}

		// Make sure render-time delays are not influencing scroll-time
		if (lNextFrame.tv_sec == 0 && lNextFrame.tv_nsec == 0)
		{
			// First time. Start timer, but don't wait.
			clock_gettime(CLOCK_MONOTONIC, &lNextFrame);
		}
		else
		{
			AddMicros(&lNextFrame, lDelaySpeed_usec);
			clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &lNextFrame, NULL);
		}
		// Swap the offscreen_canvas with canvas on vsync, avoids flickering
		lOffscreenCanvas = lMatrix->SwapOnVSync(lOffscreenCanvas);
	}

	lMatrix->Clear();
	delete lMatrix;

	delete lpDisplayManager;
	delete lpObjectGroupManager;

	return 0;
}