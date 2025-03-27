#pragma once

#include <Flaw.h>
#include <Windows.h>
#include <filesystem>

namespace flaw{
	class ContentBrowserEditor {
	public:
		ContentBrowserEditor(Application& app);

		void OnRender();

	private:
		enum class FileType {
			Directory,
			Unknown,
			Png,
			Count
		};

		void CreateIcon(FileType fileType, const char* filePath);

		void RefreshDirectory();

	private:
		Application& _app;

		std::string _contentDirectory;

		HANDLE _changeHandle;

		std::filesystem::path _currentDirectory;
		std::vector<std::filesystem::directory_entry> _directoryEntries;

		std::array<Ref<Texture>, (size_t)FileType::Count> _fileTypeIcons;
	};
}
