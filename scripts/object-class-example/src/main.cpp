#include <chrono>
#include <thread>

#include "DisplayManager.h"
#include "ObjectGroupManager.h"

int main()
{
	using namespace std::chrono;
	auto start = high_resolution_clock::now();

	ObjectGroupManagerClass *lpObjectGroupManager = new ObjectGroupManagerClass();
	DisplayManagerClass *lpDisplayManager = new DisplayManagerClass(lpObjectGroupManager);

	while (true)
	{
		std::this_thread::sleep_until(start + 1000ms);
		start = high_resolution_clock::now();
	}
	delete lpDisplayManager;
	delete lpObjectGroupManager;

	return 0;
}