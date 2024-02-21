#include <chrono>
#include <thread>
#include <signal.h>
#include <iostream>

#include "DisplayManager.h"
#include "ObjectGroupManager.h"
#include "ObjectType.h"
#include "TextObject.h"
#include "MultiObject.h"

#include "led-matrix.h"
#include "graphics.h"

using namespace rgb_matrix;
using namespace std;

#define PIXEL_WIDTH 64 * 1
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

int main()
{
	signal(SIGTERM, InterruptHandler);
	signal(SIGINT, InterruptHandler);

	using namespace chrono;
	auto start = high_resolution_clock::now();

	ObjectGroupManagerClass *lpObjectGroupManager = new ObjectGroupManagerClass();
	DisplayManagerClass *lpDisplayManager = new DisplayManagerClass(lpObjectGroupManager, PIXEL_WIDTH);

	// Customize
	int lLetterSpacing = 0;
	float lSpeed = 1.0f;
	int lDelaySpeed_usec = 1000000 / lSpeed / 10; // TODO: Figure this out lFont.CharacterWidth('W');
	Color lColor(255, 255, 255);

	// Setup Matrix
	RGBMatrix::Options lMatrixOptions;
	rgb_matrix::RuntimeOptions lRuntimeOptions;

	lMatrixOptions.rows = 32;
	lMatrixOptions.cols = 64;
	lMatrixOptions.chain_length = 1;
	lMatrixOptions.parallel = 1;
	lMatrixOptions.hardware_mapping = "adafruit-hat"; // or "adafruit-hat" depending on your setup
	lMatrixOptions.disable_hardware_pulsing = true;
	lRuntimeOptions.gpio_slowdown = 4;

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

	// TEST
	const char *lpFontFile = "../libs/rpi-rgb-led-matrix/fonts/4x6.bdf";
	rgb_matrix::Font lFont;
	if (!lFont.LoadFont(lpFontFile))
	{
		fprintf(stderr, "Couldn't load font '%s'\n", lpFontFile);
		return 1;
	}

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
					// Do nothing right now
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