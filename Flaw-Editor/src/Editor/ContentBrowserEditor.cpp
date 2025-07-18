#include "ContentBrowserEditor.h"
#include "AssetDatabase.h"
#include "EditorHelper.h"
#include "EditorEvents.h"

#include <Windows.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace flaw {
	ContentBrowserEditor::ContentBrowserEditor(Application& app)
		: _app(app)
		, _eventDispatcher(app.GetEventDispatcher())
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
		CreateIcon(FileType::Material, "Resources/Icons/MaterialFile.png");

		_changeHandle = FindFirstChangeNotification(
			_currentDirectory.wstring().c_str(),
			true,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_ACTION_ADDED | FILE_ACTION_REMOVED
		);
	}

	void ContentBrowserEditor::SetScene(const Ref<Scene>& scene) {
		_scene = scene;
	}

	void ContentBrowserEditor::OnRender() {
		ImGui::Begin("Content Browser");

		DrawImportButton();
		DrawTexture2DImportPopup();
		DrawTexture2DArrayImportPopup();
		DrawTextureCubeImportPopup();
		DrawFontImportPopup();
		DrawSoundImportPopup();
		DrawModelImportPopup();
		DrawGraphicsShaderImportPopup();

		std::filesystem::path dirHasToBeChanged;

		ImVec2 contentSize = ImGui::GetContentRegionAvail();

		const uint32_t iconMaxStride = 20;
		const float iconSize = (contentSize.x - (14.f * (iconMaxStride + 1))) / iconMaxStride;

		uint32_t remainingCount = iconMaxStride;

		if (_currentDirectory != std::filesystem::path(AssetDatabase::GetContentsDirectory())) {
			auto dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Directory]);

			ImGui::BeginGroup();
			ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
			ImGui::ImageButton("..", (ImTextureID)dxTexture->GetShaderResourceView(), { iconSize, iconSize });
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
				ImGui::ImageButton(folderName.c_str(), (ImTextureID)dxTexture->GetShaderResourceView(), { iconSize, iconSize });
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
				EditorHelper::ShowScrollingText(("##" + folderName + "ScrollingText").c_str(), folderName.c_str(), {iconSize, 24.0f}, 30.0f);
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
					else if (metadata.type == AssetType::Material) {
						dxTexture = std::static_pointer_cast<DXTexture2D>(_fileTypeIcons[(size_t)FileType::Material]);
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
				ImGui::ImageButton(filename.c_str(), (ImTextureID)dxTexture->GetShaderResourceView(), { iconSize, iconSize });
				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%s", path.generic_u8string().c_str());

					if (ImGui::IsMouseDoubleClicked(0)) {
						_eventDispatcher.Dispatch<OnDoubleClickAssetFileEvent>(path.generic_string().c_str());
					}
				}

				if (ImGui::BeginDragDropSource()) {
					const std::string fullPath = path.generic_string();
					ImGui::SetDragDropPayload("CONTENT_FILE_PATH", fullPath.c_str(), fullPath.size() + 1, ImGuiCond_Once);
					ImGui::EndDragDropSource();
				}

				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + iconSize);
				EditorHelper::ShowScrollingText(("##" + filename + "ScrollingText").c_str(), filename.c_str(), {iconSize, 24.0f}, 30.0f);
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

		if (WaitForSingleObject(_changeHandle, 0) == WAIT_OBJECT_0) {
			dirHasToBeChanged = _currentDirectory;
			FindNextChangeNotification(_changeHandle);
		}

		if (!dirHasToBeChanged.empty()) {
			_currentDirectory = dirHasToBeChanged;
			RefreshDirectory();
		}

		// Context Menu for Content Browser
		if (!ImGui::IsPopupOpen("ContextMenu") && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup("ContentBrowserContextMenu");
		}

		if (ImGui::BeginPopup("ContentBrowserContextMenu")) {
			if (ImGui::BeginMenu("New Asset")) {
				if (ImGui::MenuItem("Material")) {
					MaterialCreateSettings settings;
					settings.destPath = _currentDirectory.generic_string() + "/NewMaterial.asset";
					settings.renderMode = RenderMode::Opaque;
					settings.cullMode = CullMode::Back;
					settings.depthTest = DepthTest::Less;
					settings.depthWrite = true;
					AssetDatabase::CreateAsset(&settings);
				}

				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("New Folder")) {
				std::filesystem::path expected = _currentDirectory / "NewFolder";
				expected = FileSystem::GetUniqueFolderPath(expected.generic_string().c_str());
				std::filesystem::create_directory(expected);

				RefreshDirectory();
			}

			ImGui::EndPopup();
		}

		// TODO: modify this
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ID")) {
				entt::entity id = *(entt::entity*)payload->Data;
				Entity draggedEntity(id, _scene.get());

				PrefabCreateSettings prefabSettings;
				prefabSettings.destPath = _currentDirectory.generic_string() + "/" + draggedEntity.GetName() + ".asset";
				prefabSettings.prefabData = Prefab::ExportData(draggedEntity);
				AssetDatabase::CreateAsset(&prefabSettings);
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::End();
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

			if (ImGui::Button("Model")) {
				_openModelImportPopup = true;
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Button("Graphics Shader")) {
				_openGraphicsShaderImportPopup = true;
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

		if (_openModelImportPopup) {
			ImGui::OpenPopup("Import Model");
			_openModelImportPopup = false;
		}

		if (_openGraphicsShaderImportPopup) {
			ImGui::OpenPopup("Import Graphics Shader");
			_openGraphicsShaderImportPopup = false;
		}
	}

	void ContentBrowserEditor::DrawTexture2DImportPopup() {
		if (ImGui::BeginPopupModal("Import Texture2D", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			EditorHelper::DrawInputFilePath("Texture File", _importFilePath, "Image Files (*.png;*.jpg;*.jpeg;*.hdr)\0");

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

			EditorHelper::DrawInputText("File Name", fileName);

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
			EditorHelper::DrawInputFilePath("Font File", _importFilePath, "Font Files (*.ttf)\0");

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
			EditorHelper::DrawInputFilePath("Sound File", _importFilePath, "Sound Files (*.wav;*.ogg)\0");

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

	void ContentBrowserEditor::DrawModelImportPopup() {
		if (ImGui::BeginPopupModal("Import Model", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			EditorHelper::DrawInputFilePath("Model File", _importFilePath, "Model Files (*.fbx;*.obj)\0");

			static bool withoutSkin = false;
			ImGui::Checkbox("Without Skin", &withoutSkin);

			if (!_importFilePath.empty()) {
				if (ImGui::Button("OK")) {
					ModelImportSettings modelSettings;
					modelSettings.srcPath = _importFilePath.generic_string();
					modelSettings.destPath = _currentDirectory.generic_string() + "/" + _importFilePath.filename().replace_extension(".asset").generic_string();
					modelSettings.withoutSkin = withoutSkin;
					modelSettings.progressHandler = [](float progress) {
						Log::Info("Model import progress: %.2f%%", progress * 100.0f);
						return true;
					};

					AssetDatabase::ImportAsset(&modelSettings); 
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

	void ContentBrowserEditor::DrawGraphicsShaderImportPopup() {
		if (ImGui::BeginPopupModal("Import Graphics Shader", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			EditorHelper::DrawInputFilePath("Shader File", _importFilePath, "Shader Files (*.hlsl)\0");

			ImGui::Text("Vertex");
			ImGui::SameLine();
			static bool vertexShaderCompileFlag = false;
			ImGui::Checkbox("##VertexShader", &vertexShaderCompileFlag);

			ImGui::Text("Pixel");
			ImGui::SameLine();
			static bool pixelShaderCompileFlag = false;
			ImGui::Checkbox("##PixelShader", &pixelShaderCompileFlag);

			ImGui::Text("Geometry");
			ImGui::SameLine();
			static bool geometryShaderCompileFlag = false;
			ImGui::Checkbox("##GeometryShader", &geometryShaderCompileFlag);

			ImGui::Text("Hull");
			ImGui::SameLine();
			static bool hullShaderCompileFlag = false;
			ImGui::Checkbox("##HullShader", &hullShaderCompileFlag);

			ImGui::Text("Domain");
			ImGui::SameLine();
			static bool domainShaderCompileFlag = false;
			ImGui::Checkbox("##DomainShader", &domainShaderCompileFlag);

			if (!_importFilePath.empty()) {
				if (ImGui::Button("OK")) {
					GraphicsShaderImportSettings shaderSettings;
					shaderSettings.srcPath = _importFilePath.generic_string();
					shaderSettings.destPath = _currentDirectory.generic_string() + "/" + _importFilePath.filename().replace_extension(".asset").generic_string();
					shaderSettings.compileFlags = 0;
					shaderSettings.compileFlags |= vertexShaderCompileFlag ? ShaderCompileFlag::Vertex : 0;
					shaderSettings.compileFlags |= pixelShaderCompileFlag ? ShaderCompileFlag::Pixel : 0;
					shaderSettings.compileFlags |= geometryShaderCompileFlag ? ShaderCompileFlag::Geometry : 0;
					shaderSettings.compileFlags |= hullShaderCompileFlag ? ShaderCompileFlag::Hull : 0;
					shaderSettings.compileFlags |= domainShaderCompileFlag ? ShaderCompileFlag::Domain : 0;

					AssetDatabase::ImportAsset(&shaderSettings);
					_importFilePath.clear();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
			}

			if (ImGui::Button("Cancel")) {
				vertexShaderCompileFlag = false;
				pixelShaderCompileFlag = false;
				geometryShaderCompileFlag = false;
				hullShaderCompileFlag = false;
				domainShaderCompileFlag = false;
				_importFilePath.clear();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
}