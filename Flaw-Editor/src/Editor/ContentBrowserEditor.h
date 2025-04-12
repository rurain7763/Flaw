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
			Texture2D,
			TextureCube,
			Font,
			Scene,
			Sound,
			Count
		};

		void CreateIcon(FileType fileType, const char* filePath);

		void RefreshDirectory();

		void ShowImportPopup();

	private:
		Application& _app;

		HANDLE _changeHandle;

		std::filesystem::path _currentDirectory;
		std::vector<std::filesystem::directory_entry> _directoryEntries;

		std::array<Ref<Texture2D>, (size_t)FileType::Count> _fileTypeIcons;

		std::filesystem::path _importFilePath;
	};
}
