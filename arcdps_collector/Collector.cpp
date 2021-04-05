#include "Collector.h"
#include "Log.h"

#include <cassert>
#include <cstdio>

void Collector::Add(
	XevtcEventSource pEventSource,
	uint32_t pSeq,
	cbtevent* pEvent,
	ag* pSourceAgent,
	ag* pDestinationAgent,
	const char* pSkillname,
	uint64_t pId,
	uint64_t pRevision)
{
	int32_t paddedCount = 0;

	{
		std::lock_guard lock(mLock);

		if (mSeqStart > pSeq)
		{
			LOG("Skipping event seq %u - it happened after reset %u", pSeq, mSeqStart);
			return;
		}

		XevtcEvent* newEvent = nullptr;
		if ((pSeq - mSeqStart) < mEvents.size())
		{
			newEvent = &mEvents[pSeq];
			paddedCount = -1;
		}
		else
		{
			// Insert placeholder events for the events preceeding this one so that inserting those events does not require
			// moving elements
			while ((pSeq - mSeqStart) > mEvents.size())
			{
				XevtcEvent& temp = mEvents.emplace_back();
				memset(&temp, 0xff, sizeof(temp));

				paddedCount++;
			}

			newEvent = &mEvents.emplace_back();
		}

		memset(newEvent, 0x00, sizeof(*newEvent));
		if (pEvent != nullptr)
		{
			*static_cast<cbtevent*>(&newEvent->ev) = *pEvent;
			newEvent->ev.present = true;
		}
		else
		{
			newEvent->ev.present = false;
		}

		if (pSourceAgent != nullptr)
		{
			newEvent->source_ag.id = pSourceAgent->id;
			newEvent->source_ag.prof = pSourceAgent->prof;
			newEvent->source_ag.elite = pSourceAgent->elite;
			newEvent->source_ag.self = pSourceAgent->self;
			newEvent->source_ag.name = GetCompressedString(pSourceAgent->name);
			newEvent->source_ag.team = pSourceAgent->team;

			newEvent->source_ag.present = true;
		}
		else
		{
			newEvent->source_ag.present = false;
		}

		if (pDestinationAgent != nullptr)
		{
			newEvent->destination_ag.id = pDestinationAgent->id;
			newEvent->destination_ag.prof = pDestinationAgent->prof;
			newEvent->destination_ag.elite = pDestinationAgent->elite;
			newEvent->destination_ag.self = pDestinationAgent->self;
			newEvent->destination_ag.name = GetCompressedString(pDestinationAgent->name);
			newEvent->destination_ag.team = pDestinationAgent->team;

			newEvent->destination_ag.present = true;
		}
		else
		{
			newEvent->destination_ag.present = false;
		}

		newEvent->collector_source = pEventSource;
		newEvent->skillname = GetCompressedString(pSkillname);
		newEvent->id = pId;
		newEvent->revision = pRevision;
	}

	if (paddedCount != 0)
	{
		LOG("Inserted event id %llu sequence %u paddedCount=%i", pId, pSeq, paddedCount);
	}
}

void Collector::Reset()
{
	uint32_t oldSeq, newSeq;
	{
		std::lock_guard lock(mLock);

		oldSeq = mSeqStart;
		mSeqStart += static_cast<uint32_t>(mEvents.size());
		newSeq = mSeqStart;

		mEvents.clear();
		mStrings.clear();
	}

	LOG("Reset oldSeq %u newSeq %u", oldSeq, newSeq);
}

uint32_t Collector::Save(const char* pFilePath)
{
	XevtcHeader header;
	FILE* file = fopen(pFilePath, "wb");
	if (file == nullptr)
	{
		LOG("Opening '%s' failed - %u", pFilePath, errno);
		return errno;
	}

	{
		std::lock_guard lock(mLock);

		header.Version = 1;
		header.StringCount = static_cast<uint32_t>(mStrings.size());
		header.EventCount = static_cast<uint32_t>(mEvents.size());
		size_t written = fwrite(&header, sizeof(header), 1, file);
		if (written != 1)
		{
			LOG("Writing header to '%s' failed - %zu/1 - error %u", pFilePath, written, errno);
			return errno;
		}

		// Construct a vector of the strings in the order we want them to be persisted
		std::vector<std::string> stringsVector{mStrings.size()};
		for (const auto& [string, index] : mStrings)
		{
			stringsVector[index - 1] = string;
		}

		// Persist the strings with a uint16_t header each representing their size
		for (uint32_t i = 0; i < stringsVector.size(); i++)
		{
			uint16_t size = static_cast<uint16_t>(stringsVector[i].size());
			assert(stringsVector[i].size() == size); // make sure value wasn't truncated

			written = fwrite(&size, sizeof(size), 1, file);
			if (written != 1)
			{
				LOG("Writing string %u size to '%s' failed - %zu/1 - error %u", i, pFilePath, written, errno);
				return errno;
			}

			written = fwrite(stringsVector[i].c_str(), sizeof(char), size, file);
			if (written != size)
			{
				LOG("Writing string %u to '%s' failed - %zu/%hu - error %u", i, pFilePath, written, size, errno);
				return errno;
			}
		}

		written = fwrite(mEvents.data(), sizeof(XevtcEvent), mEvents.size(), file);
		if (written != mEvents.size())
		{
			LOG("Writing events to '%s' failed - %zu/%zu - error %u", pFilePath, written, mEvents.size(), errno);
			return errno;
		}
	}

	if (fclose(file) == EOF)
	{
		LOG("Closing '%s' failed - %u", pFilePath, errno);
		return errno;
	}

	LOG("Wrote %u strings, %u events, to %s", header.StringCount, header.EventCount, pFilePath);
	return 0;
}

size_t Collector::GetEventCount()
{
	{
		std::lock_guard lock(mLock);
		return mEvents.size();
	}
}

// Requires mutex to already be held
XevtcString Collector::GetCompressedString(const char* pString)
{
	if (pString == nullptr)
	{
		return XevtcString{UINT32_MAX};
	}

	std::string tempString{pString};

	auto [iterator, inserted] = mStrings.try_emplace(std::move(tempString), 0);
	if (inserted == true)
	{
		iterator->second = static_cast<uint32_t>(mStrings.size());
	}

	assert(iterator->second != 0);
	return XevtcString{iterator->second};
}
