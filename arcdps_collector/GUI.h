#pragma once
#include "Collector.h"

class GUI
{
public:
	GUI(Collector& pCollector);

	void Display();
	void DisplayOptions();

private:
	Collector& mCollector;

	bool mDisplayed = false;
	bool mCreateMissingPath = true;
	char mInputPath[256] = "addons\\arcdps_collector_logs\\";
	char mLastError[4096] = {};
};