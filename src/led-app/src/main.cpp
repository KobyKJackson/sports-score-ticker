#include <chrono>
#include <thread>
#include <signal.h>

#include "DisplayManager.h"
#include "ObjectGroupManager.h"
#include "ObjectType.h"

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

	using namespace std::chrono;
	auto start = high_resolution_clock::now();

	ObjectGroupManagerClass *lpObjectGroupManager = new ObjectGroupManagerClass();
	DisplayManagerClass *lpDisplayManager = new DisplayManagerClass(lpObjectGroupManager, PIXEL_WIDTH);

	// Customize
	int lLetterSpacing = 0;
	float lSpeed = 7.0f;
	int lDelaySpeed_usec = 1000000 / lSpeed / 10; // TODO: Figure this out lFont.CharacterWidth('W');

	struct timespec lNextFrame = {0, 0};
	uint64_t lFrameCounter = 0;

	while (!IsInterruptReceived)
	{
		++lFrameCounter;
		// lOffscreenCanvas->Fill(0, 0, 0);

		for (auto &lpObjectGroup : lpDisplayManager->GetDisplayObjects())
		{
			lpObjectGroup.IncrementXPosition();

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
					// rgb_matrix::DrawText(lOffscreenCanvas, lFont, lX, lY + lFont.baseline(), lColor, NULL, TEXT.c_str(), lLetterSpacing);
				}
				break;

				case OBJECT_TYPE::MULTI:
				{
					// Do nothing right now
					/*
					MultiObjectClass *lpMultiObject = static_cast<MultiObjectClass *>(lpObject);
					for (uint8_t k = 0; k < lpMultiObject->GetNumberOfObjects(); k++)
					{
						ObjectTypeClass *lpObjectType = lpMultiObject->GetByIndex(k);
						switch (lpObjectType->GetObjectType())
						{
							case OBJECT_TYPE::IMAGE:
							{
								ImageObjectClass *lpImageObject = static_cast<ImageObjectClass *>(lpObjectType);
								cout << "data: " << lpImageObject->GetValue() << endl;
							}
							break;

							case OBJECT_TYPE::TEXT:
							{
								TextObjectClass *lpTextObject = static_cast<TextObjectClass *>(lpObjectType);
								cout << "data: " << lpTextObject->GetValue() << endl;
							}
							break;

							default:
								break;
						}
					}
					*/
				}
				break;

				default:
					break;
				}
			}
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
			// TODO: Comment back in: clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &lNextFrame, NULL);
		}
		// Swap the offscreen_canvas with canvas on vsync, avoids flickering
		// lOffscreenCanvas = lMatrix->SwapOnVSync(lOffscreenCanvas);
	}

	// lMatrix->Clear();
	// delete lMatrix;

	delete lpDisplayManager;
	delete lpObjectGroupManager;

	return 0;
}