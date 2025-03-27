#pragma once

#include <Flaw.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace flaw {
	class LogEditor;

	class CustomLogSink : public LogSink {
	public:
		CustomLogSink(LogEditor* logEditor);

		void Log(const char* message) override;
		void Flush() override;

	private:
		LogEditor* _logEditor;
	};

	class LogEditor {
	public:
		LogEditor();

		void OnRender();

        void Clear();
        void AddLog(const char* fmt, ...);

	private:
		ImGuiTextBuffer     Buf;
		ImGuiTextFilter     Filter;
		ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
		bool                AutoScroll;  // Keep scrolling if already at the bottom.

		bool _open = true;
	};
}

