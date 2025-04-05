#include "ContentBrowserEditor.h"
#include "AssetDatabase.h"

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
		CreateIcon(FileType::Texture2D, "Resources/Icons/Texture2D.png");

		_changeHandle = FindFirstChangeNotification(
			_currentDirectory.wstring().c_str(),
			true,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_ACTION_ADDED | FILE_ACTION_REMOVED
		);
	}

	void ContentBrowserEditor::OnRender() {
		ImGui::Begin("Content Browser");

		if (ImGui::Button("Import")) {
			std::filesystem::path filePath = FileDialogs::OpenFile(Platform::GetPlatformContext(), "All Files (*.*)\0");
			if (!filePath.empty()) {
				std::filesystem::path destPath = _currentDirectory.generic_string() + "/" + filePath.filename().replace_extension(".asset").generic_string();
				AssetDatabase::ImportAsset(filePath.generic_string().c_str(), destPath.generic_string().c_str());
			}
		}

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
				}

				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + iconSize);
				ImGui::TextWrapped("%s", folderName.c_str());
				ImGui::PopTextWrapPos();
				ImGui::EndGroup();
			}
			else {
				if (path.extension() != ".asset") {
					continue;
				}

				AssetMetadata metadata;
				if (!AssetDatabase::GetAssetMetadata(path.generic_string().c_str(), metadata)) {
					continue;
				}

				Ref<DXTexture2D> dxTexture = nullptr;
				if (metadata.type == AssetType::Texture2D) {
					dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Texture2D]);
				}
				else {
					dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Unknown]);
				}

				const std::string filename = path.filename().generic_u8string();

				ImGui::BeginGroup();
				ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
				ImGui::ImageButton(filename.c_str(), (ImTextureID)dxTexture->GetShaderResourceView().Get(), { iconSize, iconSize });
				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%s", path.generic_u8string().c_str());

					if (ImGui::IsMouseDoubleClicked(0)) {
						
					}
				}

				if (ImGui::BeginDragDropSource()) {
					const std::string fullPath = path.generic_string();
					ImGui::SetDragDropPayload("CONTENT_FILE_PATH", fullPath.c_str(), fullPath.size() + 1, ImGuiCond_Once);
					ImGui::EndDragDropSource();
				}

				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + iconSize);
				ImGui::TextWrapped("%s", filename.c_str());
				ImGui::PopTextWrapPos();
				ImGui::EndGroup();
			}

			if (--remainingCount == 0) {
				remainingCount = iconMaxStride;
				ImGui::NewLine();
			}
			else {
				ImGui::SameLine();
			}
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
		Image iconImg(filePath);

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
}