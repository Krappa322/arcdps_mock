#include "GUI.h"
#include "Log.h"

#include "imgui.h"

#include <direct.h>
#include <cstring>

namespace
{
void CreateMissingPath(const char* pFilePath)
{
	char buffer[4096];
	snprintf(buffer, sizeof(buffer), "%s", pFilePath);

	char* curPos = strchr(buffer, '\\');
	while (curPos != nullptr)
	{
		*curPos = '\0';
		if (_mkdir(buffer) == -1 && errno != EEXIST)
		{
			LOG("Creating %s failed - errno %u GetLastError %u", buffer, errno, GetLastError());
		}

		*curPos = '\\';
		curPos = strchr(curPos + 1, '\\');
	}
}
};

GUI::GUI(Collector& pCollector)
	: mCollector(pCollector)
{
}

void GUI::Display()
{
	if (mDisplayed == false)
	{
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(600, 250), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Event Collector", &mDisplayed, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav) == true)
	{
		ImGui::Text("%zu events collected", mCollector.GetEventCount());

		if (ImGui::Button("Restart") == true)
		{
			mCollector.Reset();
		}
		
		ImGui::SameLine();
		if (ImGui::Button("Save") == true)
		{
			// Generate filename in a format like 2021-04-04_14-50-45_315.xevtc
			char timeBuffer[128];
			int64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			int64_t seconds = milliseconds / 1000;
			milliseconds = milliseconds % 1000;
			int64_t writtenChars = std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d_%H-%M-%S", std::localtime(&seconds));
			assert(writtenChars >= 0);

			char pathBuffer[4096];
			writtenChars = snprintf(pathBuffer, sizeof(pathBuffer) - 1, "%s\\%s_%03lli.xevtc", mInputPath, timeBuffer, milliseconds);
			assert(writtenChars >= 0);
			assert(writtenChars < sizeof(pathBuffer) - 1);

			if (mCreateMissingPath == true)
			{
				CreateMissingPath(pathBuffer);
			}

			uint32_t result = mCollector.Save(pathBuffer);
			if (result != 0)
			{
				size_t pos = snprintf(mLastError, sizeof(mLastError), "Failed writing to %s - errno %u GetLastError %u - ", pathBuffer, result, GetLastError());
				FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					mLastError + pos, (sizeof(mLastError) - pos), NULL);
			}
			else
			{
				mLastError[0] = '\0';
			}
		}

		ImGui::Checkbox("Create missing path", &mCreateMissingPath);

		float oldPosY = ImGui::GetCursorPosY();

		ImGui::SetCursorPosY(oldPosY + ImGui::GetStyle().FramePadding.y);
		ImGui::Text("Logs folder");

		ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
		ImGui::SetCursorPosY(oldPosY);

		ImGui::PushItemWidth(-1);
		ImGui::InputText("##mInputPath", mInputPath, sizeof(mInputPath));
		ImGui::PopItemWidth();

		if (mLastError[0] != '\0')
		{
			ImGui::TextWrapped("%s", mLastError);
		}
	}

	ImGui::End();
}

void GUI::DisplayOptions()
{
	ImGui::Checkbox("Event Collector Options", &mDisplayed);
}
