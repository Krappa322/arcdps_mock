#pragma once
#include "Xevtc.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <vector>

class Collector
{
public:
	void Add(
		XevtcEventSource pEventSource,
		uint32_t pSeq,
		cbtevent* pEvent,
		ag* pSourceAgent,
		ag* pDestinationAgent,
		const char* pSkillname,
		uint64_t pId,
		uint64_t pRevision);

	void Reset();
	uint32_t Save(const char* pFilePath);

	size_t GetEventCount();

private:
	XevtcString GetCompressedString(const char* pString);

	std::mutex mLock;

	// Strings are deduplicated when the string is added. The string is stored here with an accompanying index and in all
	// other places it's stored with just the index, which can then be mapped back to the string when necessary
	std::map<std::string, uint32_t> mStrings;

	uint32_t mSeqStart = 0;
	std::vector<XevtcEvent> mEvents;
};