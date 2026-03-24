#pragma once
#include "Handler.hpp"

void Loop() {
	static int64_t previousPlaceId = -1;
	static uintptr_t previousDataModel = 0;

	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		uintptr_t currentDataModel = Scheduler->DataModel();
		if (!currentDataModel)
			continue;

		int64_t currentPlaceId = Scheduler->GetPlaceID();
		if (!currentPlaceId)
			continue;

		if (currentPlaceId == previousPlaceId &&
			currentDataModel == previousDataModel)
		{
			continue;
		}

		previousPlaceId = currentPlaceId;
		previousDataModel = currentDataModel;

		PreviousDM = currentDataModel;

		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		TaskScheduler->Init();
	}
}

void CTPService::Initialize() {
    std::thread(Loop).detach();
}