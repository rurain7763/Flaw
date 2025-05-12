#include "ContentBrowserEditor.h"
#include "AssetDatabase.h"
#include "EditorHelper.h"

#include <Windows.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace flaw {
	ContentBrowserEditor::ContentBrowserEditor(Application& app)
		: _app(app)
	{
		_currentDirectory = AssetDatabase::GetContentsDirectory();

		RefreshDirectory();

		CreateIcon(FileType::Directory, "Resources/Icons/Directory.png");
		CreateIcon(FileType::Unknown, "Resources/Icons/UnknownFile.png");
		CreateIcon(FileType::Texture2D, "Resources/Icons/Texture2DFile.png");
		CreateIcon(FileType::Texture2DArray, "Resources/Icons/Texture2DArrayFile.png");
		CreateIcon(FileType::TextureCube, "Resources/Icons/TextureCubeFile.png");
		CreateIcon(FileType::Font, "Resources/Icons/FontFile.png");
		CreateIcon(FileType::Scene, "Resources/Icons/SceneFile.png");
		CreateIcon(FileType::Sound, "Resources/Icons/SoundFile.png");

		_changeHandle = FindFirstChangeNotification(
			_currentDirectory.wstring().c_str(),
			true,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_ACTION_ADDED | FILE_ACTION_REMOVED
		);
	}

	// TODO: 이 코드는 다른 곳으로 이동해야 함
	static void ShowScrollingText(const char* label, const char* text, ImVec2 boxSize, float scrollSpeed = 30.0f) {
		ImGui::BeginGroup();

		ImVec2 pos = ImGui::GetCursorScreenPos();

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImGuiID id = window->GetID(label);
		ImVec2 textSize = ImGui::CalcTextSize(text);

		float itemHeight = textSize.y;
		ImVec2 textRegionSize = ImVec2(boxSize.x, itemHeight);

		// Invisible button to detect hover
		ImGui::InvisibleButton(label, textRegionSize);
		bool isHovered = ImGui::IsItemHovered();

		float availableWidth = textRegionSize.x;
		float overflow = textSize.x - availableWidth;

		float scrollOffset = 0.0f;

		if (overflow > 0.0f) {
			static std::unordered_map<ImGuiID, float> scrollOffsets;
			static std::unordered_map<ImGuiID, double> lastResetTime;

			double currentTime = ImGui::GetTime();

			if (!isHovered) {
				// Reset animation when not hovered
				scrollOffsets[id] = 0.0f;
				lastResetTime[id] = currentTime;
			}
			else {
				const float pauseDuration = 3.0f; // Pause duration at the end of the scroll

				float totalScrollTime = overflow / scrollSpeed;
				float elapsed = (float)(currentTime - lastResetTime[id]);

				float cycleTime = totalScrollTime + pauseDuration;
				float t = fmodf(elapsed, cycleTime);

				if (t < totalScrollTime) {
					scrollOffset = t * scrollSpeed;
				}
				else {
					scrollOffset = overflow; // pause at end
				}

				scrollOffsets[id] = scrollOffset;
			}

			scrollOffset = scrollOffsets[id];
		}

		// Clip and draw
		ImGui::SetCursorScreenPos(pos);
		ImGui::PushClipRect(pos, ImVec2(pos.x + textRegionSize.x, pos.y + textRegionSize.y), true);
		ImGui::SetCursorScreenPos(ImVec2(pos.x - scrollOffset, pos.y));
		ImGui::TextUnformatted(text);
		ImGui::PopClipRect();

		ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + textRegionSize.y));
		ImGui::EndGroup();
	}

	void ContentBrowserEditor::OnRender() {
		ImGui::Begin("Content Browser");

		DrawImportButton();
		DrawTexture2DImportPopup();
		DrawTexture2DArrayImportPopup();
		DrawTextureCubeImportPopup();
		DrawFontImportPopup();
		DrawSoundImportPopup();

		std::filesystem::path dirHasToBeChanged;

		ImVec2 contentSize = ImGui::GetContentRegionAvail();

		const uint32_t iconMaxStride = 20;
		const float iconSize = (contentSize.x - (14.f * (iconMaxStride + 1))) / iconMaxStride;

		uint32_t remainingCount = iconMaxStride;

		if (_currentDirectory != std::filesystem::path(AssetDatabase::GetContentsDirectory())) {
			auto dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Directory]);

			ImGui::BeginGroup();
			ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
			ImGui::ImageButton("..", (ImTextureID)dxTexture->GetShaderResourceView().Get(), { iconSize, iconSize });
			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s", _currentDirectory.parent_path().generic_u8string().c_str());

				if (ImGui::IsMouseDoubleClicked(0)) {
					dirHasToBeChanged = _currentDirectory.parent_path();
				}
			}

			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + iconSize);
			ImGui::TextWrapped("..");
			ImGui::PopTextWrapPos();
			ImGui::EndGroup();

			--remainingCount;
			ImGui::SameLine();
		}

		static std::filesystem::path fileExplorerPath;
		for (auto& dir : _directoryEntries) {
			std::filesystem::path path = dir.path();

			if (dir.is_directory()) {
				auto dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Directory]);
				const std::string folderName = path.filename().generic_u8string();

				ImGui::BeginGroup(); // 그룹화하여 아이콘과 텍스트 정렬
				ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
				ImGui::ImageButton(folderName.c_str(), (ImTextureID)dxTexture->GetShaderResourceView().Get(), { iconSize, iconSize });
				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%s", path.generic_u8string().c_str()); // 전체 경로 툴팁 표시

					if (ImGui::IsMouseDoubleClicked(0)) {
						dirHasToBeChanged = path;
					}

					if (ImGui::IsMouseClicked(1)) {
						fileExplorerPath = path.generic_u8string();
						ImGui::OpenPopup("ContextMenu");
					}
				}

				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + iconSize);
				ShowScrollingText(("##" + folderName + "ScrollingText").c_str(), folderName.c_str(), {iconSize, 24.0f}, 30.0f);
				ImGui::PopTextWrapPos();
				ImGui::EndGroup();
			}
			else if (path.extension() == ".asset" || path.extension() == ".scene") {
				Ref<DXTexture2D> dxTexture = nullptr;
				if (path.extension() == ".asset") {
					AssetMetadata metadata;
					if (!AssetDatabase::GetAssetMetadata(path.generic_string().c_str(), metadata)) {
						continue;
					}

					if (metadata.type == AssetType::Texture2D) {
						dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Texture2D]);
					}
					else if (metadata.type == AssetType::Texture2DArray) {
						dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Texture2DArray]);
					}
					else if (metadata.type == AssetType::TextureCube) {
						dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::TextureCube]);
					}
					else if (metadata.type == AssetType::Font) {
						dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Font]);
					}
					else if (metadata.type == AssetType::Sound) {
						dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Sound]);
					}
					else {
						dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Unknown]);
					}
				}
				else if (path.extension() == ".scene") {
					dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Scene]);
				}

				const std::string filename = path.filename().generic_u8string();

				ImGui::BeginGroup();
				ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
				ImGui::ImageButton(filename.c_str(), (ImTextureID)dxTexture->GetShaderResourceView().Get(), { iconSize, iconSize });
				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%s", path.generic_u8string().c_str());
				}

				if (ImGui::BeginDragDropSource()) {
					const std::string fullPath = path.generic_string();
					ImGui::SetDragDropPayload("CONTENT_FILE_PATH", fullPath.c_str(), fullPath.size() + 1, ImGuiCond_Once);
					ImGui::EndDragDropSource();
				}

				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + iconSize);
				ShowScrollingText(("##" + filename + "ScrollingText").c_str(), filename.c_str(), {iconSize, 24.0f}, 30.0f);
				ImGui::PopTextWrapPos();
				ImGui::EndGroup();
			}
			else {
				continue;
			}

			if (--remainingCount == 0) {
				remainingCount = iconMaxStride;
				ImGui::NewLine();
			}
			else {
				ImGui::SameLine();
			}
		}

		if (ImGui::BeginPopup("ContextMenu")) {
			if (ImGui::MenuItem("Open Folder In File Explorer")) {
				std::filesystem::path openPath = std::filesystem::is_directory(fileExplorerPath) ? fileExplorerPath : fileExplorerPath.parent_path();
#ifdef _WIN32
				ShellExecuteW(NULL, L"open", openPath.wstring().c_str(), NULL, NULL, SW_SHOW);
#endif
			}
			ImGui::EndPopup();
		}

		ImGui::End();

		if (WaitForSingleObject(_changeHandle, 0) == WAIT_OBJECT_0) {
			dirHasToBeChanged = _currentDirectory;
			FindNextChangeNotification(_changeHandle);
		}

		if (!dirHasToBeChanged.empty()) {
			_currentDirectory = dirHasToBeChanged;
			RefreshDirectory();
		}
	}

	void ContentBrowserEditor::CreateIcon(FileType fileType, const char* filePath) {
		Image iconImg(filePath, 4);

		Texture2D::Descriptor desc = {};
		desc.format = PixelFormat::RGBA8;
		desc.width = iconImg.Width();
		desc.height = iconImg.Height();
		desc.data = iconImg.Data().data();
		desc.wrapS = Texture2D::Wrap::ClampToEdge;
		desc.wrapT = Texture2D::Wrap::ClampToEdge;
		desc.minFilter = Texture2D::Filter::Linear;
		desc.magFilter = Texture2D::Filter::Linear;
		desc.usage = UsageFlag::Static;
		desc.bindFlags = BindFlag::ShaderResource;

		_fileTypeIcons[(size_t)fileType] = Graphics::GetGraphicsContext().CreateTexture2D(desc);
	}

	void ContentBrowserEditor::RefreshDirectory() {
		_directoryEntries.clear();
		for (auto& dir : std::filesystem::directory_iterator(_currentDirectory)) {
			_directoryEntries.push_back(dir);
		}
	}

	void ContentBrowserEditor::DrawImportButton() {
		if (ImGui::Button("Import")) {
			ImGui::OpenPopup("Select import type");
		}

		if (ImGui::BeginPopupModal("Select import type", nullptr)) {
			if (ImGui::Button("Texture2D")) {
				_openTexture2DImportPopup = true;
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Button("Texture2DArray")) {
				_openTexture2DArrayImportPopup = true;
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Button("TextureCube")) {
				_openTextureCubeImportPopup = true;
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Button("Font")) {
				_openFontImportPopup = true;
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Button("Sound")) {
				_openSoundImportPopup = true;
				ImGui::CloseCurrentPopup();
			}

			ImGui::Separator();

			if (ImGui::Button("Cancel")) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (_openTexture2DImportPopup) {
			ImGui::OpenPopup("Import Texture2D");
			_openTexture2DImportPopup = false;
		}

		if (_openTexture2DArrayImportPopup) {
			ImGui::OpenPopup("Import Texture2DArray");
			_openTexture2DArrayImportPopup = false;
		}

		if (_openTextureCubeImportPopup) {
			ImGui::OpenPopup("Import TextureCube");
			_openTextureCubeImportPopup = false;
		}

		if (_openFontImportPopup) {
			ImGui::OpenPopup("Import Font");
			_openFontImportPopup = false;
		}

		if (_openSoundImportPopup) {
			ImGui::OpenPopup("Import Sound");
			_openSoundImportPopup = false;
		}
	}

	void ContentBrowserEditor::DrawTexture2DImportPopup() {
		if (ImGui::BeginPopupModal("Import Texture2D", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (!_importFilePath.empty()) {
				ImGui::Text("Selected file: %s", _importFilePath.filename().generic_u8string().c_str());
			}
			else {
				ImGui::Text("No file selected");
			}

			ImGui::SameLine();

			if (ImGui::Button("...")) {
				std::filesystem::path filePath = FileDialogs::OpenFile(Platform::GetPlatformContext(), "Image Files (*.png;*.jpg;*.jpeg;*.hdr)\0");
				if (!filePath.empty()) {
					_importFilePath = filePath;
				}
			}

			if (!_importFilePath.empty()) {
				if (ImGui::Button("OK")) {
					Texture2DImportSettings textureSettings;
					textureSettings.srcPath = _importFilePath.generic_string();
					textureSettings.destPath = _currentDirectory.generic_string() + "/" + _importFilePath.filename().replace_extension(".asset").generic_string();
					textureSettings.usageFlags = UsageFlag::Static;
					textureSettings.bindFlags = BindFlag::ShaderResource;
					textureSettings.accessFlags = 0;

					AssetDatabase::ImportAsset(&textureSettings);
					_importFilePath.clear();
					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();
			}

			if (ImGui::Button("Cancel")) {
				_importFilePath.clear();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void ContentBrowserEditor::DrawTexture2DArrayImportPopup() {
		if (ImGui::BeginPopupModal("Import Texture2DArray", nullptr)) {
			static std::string fileName;
			static std::vector<std::filesystem::path> imagePaths;

			EditorHelper::DrawImputText("File Name", fileName);

			for (int32_t i = 0; i < imagePaths.size(); ++i) {
				ImGui::Text("%s", imagePaths[i].filename().generic_u8string().c_str());
				
				ImGui::SameLine();
				if (ImGui::Button(("...##" + std::to_string(i)).c_str())) {
					std::filesystem::path filePath = FileDialogs::OpenFile(Platform::GetPlatformContext(), "Image Files (*.png;*.jpg;*.jpeg;*.hdr)\0");
					if (!filePath.empty()) {
						imagePaths[i] = filePath;
					}
				}
				
				ImGui::SameLine();
				if (ImGui::Button(("-##" + std::to_string(i)).c_str())) {
					imagePaths.erase(imagePaths.begin() + i);
					--i;
				}
			}

			if (ImGui::Button("+")) {
				imagePaths.resize(imagePaths.size() + 1);
			}

			if (!imagePaths.empty()) {
				if (ImGui::Button("OK")) {
					Texture2DArrayImportSettings textureSettings;	
					std::transform(imagePaths.begin(), imagePaths.end(), std::back_inserter(textureSettings.srcPaths), [](const std::filesystem::path& path) { return path.generic_string(); });
					textureSettings.destPath = _currentDirectory.generic_string() + "/" + fileName + ".asset";
					textureSettings.usageFlags = UsageFlag::Static;
					textureSettings.bindFlags = BindFlag::ShaderResource;
					textureSettings.accessFlags = 0;

					AssetDatabase::ImportAsset(&textureSettings);
					imagePaths.clear();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
			}

			if (ImGui::Button("Cancel")) {
				imagePaths.clear();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void ContentBrowserEditor::DrawTextureCubeImportPopup() {
		if (ImGui::BeginPopupModal("Import TextureCube", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (!_importFilePath.empty()) {
				ImGui::Text("Selected file: %s", _importFilePath.filename().generic_u8string().c_str());
			}
			else {
				ImGui::Text("No file selected");
			}

			ImGui::SameLine();
			if (ImGui::Button("...")) {
				std::filesystem::path filePath = FileDialogs::OpenFile(Platform::GetPlatformContext(), "Image Files (*.png;*.jpg;*.jpeg;*.hdr)\0");
				if (!filePath.empty()) {
					_importFilePath = filePath;
				}
			}

			if (!_importFilePath.empty()) {
				if (ImGui::Button("OK")) {
					TextureCubeImportSettings textureSettings;
					textureSettings.srcPath = _importFilePath.generic_string();
					textureSettings.destPath = _currentDirectory.generic_string() + "/" + _importFilePath.filename().replace_extension(".asset").generic_string();
					textureSettings.usageFlags = UsageFlag::Static;
					textureSettings.bindFlags = BindFlag::ShaderResource;
					textureSettings.accessFlags = 0;

					AssetDatabase::ImportAsset(&textureSettings);
					_importFilePath.clear();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
			}

			if (ImGui::Button("Cancel")) {
				_importFilePath.clear();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void ContentBrowserEditor::DrawFontImportPopup() {
		if (ImGui::BeginPopupModal("Import Font", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (!_importFilePath.empty()) {
				ImGui::Text("Selected file: %s", _importFilePath.filename().generic_u8string().c_str());
			}
			else {
				ImGui::Text("No file selected");
			}

			ImGui::SameLine();

			if (ImGui::Button("...")) {
				std::filesystem::path filePath = FileDialogs::OpenFile(Platform::GetPlatformContext(), "Font Files (*.ttf)\0");
				if (!filePath.empty()) {
					_importFilePath = filePath;
				}
			}

			if (!_importFilePath.empty()) {
				if (ImGui::Button("OK")) {
					FontImportSettings fontSettings;
					fontSettings.srcPath = _importFilePath.generic_string();
					fontSettings.destPath = _currentDirectory.generic_string() + "/" + _importFilePath.filename().replace_extension(".asset").generic_string();
					
					AssetDatabase::ImportAsset(&fontSettings);
					_importFilePath.clear();
					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();
			}

			if (ImGui::Button("Cancel")) {
				_importFilePath.clear();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void ContentBrowserEditor::DrawSoundImportPopup() {
		if (ImGui::BeginPopupModal("Import Sound", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (!_importFilePath.empty()) {
				ImGui::Text("Selected file: %s", _importFilePath.filename().generic_u8string().c_str());
			}
			else {
				ImGui::Text("No file selected");
			}

			ImGui::SameLine();

			if (ImGui::Button("...")) {
				std::filesystem::path filePath = FileDialogs::OpenFile(Platform::GetPlatformContext(), "Sound Files (*.wav;*.ogg)\0");
				if (!filePath.empty()) {
					_importFilePath = filePath;
				}
			}

			if (!_importFilePath.empty()) {
				if (ImGui::Button("OK")) {
					SoundImportSettings soundSettings;
					soundSettings.srcPath = _importFilePath.generic_string();
					soundSettings.destPath = _currentDirectory.generic_string() + "/" + _importFilePath.filename().replace_extension(".asset").generic_string();
					AssetDatabase::ImportAsset(&soundSettings);
					_importFilePath.clear();
					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();
			}

			if (ImGui::Button("Cancel")) {
				_importFilePath.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}
}