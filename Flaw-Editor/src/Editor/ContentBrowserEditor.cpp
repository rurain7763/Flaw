#include "ContentBrowserEditor.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace flaw {
	ContentBrowserEditor::ContentBrowserEditor(Application& app) 
		: _app(app)
	{
		auto projectConfig = Project::GetConfig();
		_contentDirectory = projectConfig.path + "/Contents";
		_currentDirectory = _contentDirectory;

		RefreshDirectory();

		CreateIcon(FileType::Directory, "Resources/Icons/Directory.png");
		CreateIcon(FileType::Unknown, "Resources/Icons/UnknownFile.png");
		CreateIcon(FileType::Png, "Resources/Icons/PngFile.png");

		_changeHandle = FindFirstChangeNotification(
			_currentDirectory.generic_string().c_str(),
			true,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_ACTION_ADDED | FILE_ACTION_REMOVED
		);
	}

	void ContentBrowserEditor::OnRender() {
		ImGui::Begin("Content Browser");

		std::filesystem::path dirHasToBeChanged;

		ImVec2 contentSize = ImGui::GetContentRegionAvail();

		const uint32_t iconMaxStride = 20;
		const float iconSize = (contentSize.x - (14.f * (iconMaxStride + 1))) / iconMaxStride;

		uint32_t remainingCount = iconMaxStride;

		if (_currentDirectory != std::filesystem::path(_contentDirectory)) {
			auto dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Directory]);

			ImGui::BeginGroup();
			ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
			ImGui::ImageButton("..", (ImTextureID)dxTexture->GetShaderResourceView().Get(), { iconSize, iconSize });
			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s", _currentDirectory.parent_path().string().c_str());

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
			if (dir.is_directory()) {
				auto dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Directory]);
				std::string folderName = dir.path().filename().generic_string();

				ImGui::BeginGroup(); // 그룹화하여 아이콘과 텍스트 정렬
				ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
				ImGui::ImageButton(folderName.c_str(), (ImTextureID)dxTexture->GetShaderResourceView().Get(), { iconSize, iconSize });
				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%s", dir.path().string().c_str()); // 전체 경로 툴팁 표시

					if (ImGui::IsMouseDoubleClicked(0)) {
						dirHasToBeChanged = dir.path();
					}
				}

				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + iconSize);
				ImGui::TextWrapped("%s", folderName.c_str());
				ImGui::PopTextWrapPos();
				ImGui::EndGroup();
			}
			else {
				const std::string filename = dir.path().filename().generic_string();
				Ref<DXTexture2D> dxTexture = nullptr;

				if (dir.path().extension() == ".png") {
					dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Png]);
				}
				else {
					dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Unknown]);
				}

				ImGui::BeginGroup();
				ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
				ImGui::ImageButton(filename.c_str(), (ImTextureID)dxTexture->GetShaderResourceView().Get(), { iconSize, iconSize });
				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%s", dir.path().string().c_str());

					if (ImGui::IsMouseDoubleClicked(0)) {
						
					}
				}

				if (ImGui::BeginDragDropSource()) {
					const std::string fullPath = dir.path().generic_string();
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
		auto& gContext = _app.GetGraphicsContext();

		Image iconImg(filePath);

		Texture::Descriptor desc = {};
		desc.type = Texture::Type::Texture2D;
		desc.format = PixelFormat::RGBA8;
		desc.width = iconImg.Width();
		desc.height = iconImg.Height();
		desc.data = iconImg.Data().data();
		desc.wrapS = Texture::Wrap::ClampToEdge;
		desc.wrapT = Texture::Wrap::ClampToEdge;
		desc.minFilter = Texture::Filter::Linear;
		desc.magFilter = Texture::Filter::Linear;
		desc.usage = UsageFlag::Static;
		desc.bindFlags = BindFlag::ShaderResource;

		_fileTypeIcons[(size_t)fileType] = gContext.CreateTexture2D(desc);
	}

	void ContentBrowserEditor::RefreshDirectory() {
		_directoryEntries.clear();
		for (auto& dir : std::filesystem::directory_iterator(_currentDirectory)) {
			_directoryEntries.push_back(dir);
		}
	}
}