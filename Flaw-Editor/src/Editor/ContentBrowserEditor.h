#pragma once

#include <Flaw.h>
#include <Windows.h>
#include <filesystem>

namespace flaw{
	class ContentBrowserEditor {
	public:
		ContentBrowserEditor(Application& app);

		void OnRender();

		std::string GetCurrentDir() const { return _currentDirectory.generic_string(); }

	private:
		enum class FileType {
			Directory,
			Unknown,
			Texture2D,
			Texture2DArray,
			TextureCube,
			Font,
			Scene,
			Sound,
			Count
		};

		void CreateIcon(FileType fileType, const char* filePath);

		void RefreshDirectory();

		void DrawImportButton();
		void DrawTexture2DImportPopup();
		void DrawTexture2DArrayImportPopup();
		void DrawTextureCubeImportPopup();
		void DrawFontImportPopup();
		void DrawSoundImportPopup();
		void DrawModelImportPopup();
		void DrawGraphicsShaderImportPopup();

	private:
		Application& _app;

		HANDLE _changeHandle;

		std::filesystem::path _currentDirectory;
		std::vector<std::filesystem::directory_entry> _directoryEntries;

		std::array<Ref<Texture2D>, (size_t)FileType::Count> _fileTypeIcons;

		std::filesystem::path _importFilePath;
		bool _openTexture2DImportPopup = false;
		bool _openTexture2DArrayImportPopup = false;
		bool _openTextureCubeImportPopup = false;
		bool _openFontImportPopup = false;
		bool _openSoundImportPopup = false;
		bool _openModelImportPopup = false;
		bool _openGraphicsShaderImportPopup = false;
	};
}
