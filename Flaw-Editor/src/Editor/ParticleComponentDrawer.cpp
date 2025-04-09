#include "ParticleComponentDrawer.h"

namespace flaw {
	template<typename T>
	static bool DrawModule(
		const char* label, 
		ParticleSystem& system, 
		Entity entity, 
		ParticleComponent& component, 
		ParticleComponent::ModuleType moduleType, 
		std::function<bool(T& module)> drawFunc) 
	{
		bool dirty = false;

		ImGui::BeginChild((std::string("##") + label + " Module").c_str(), ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);
		
		if (ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) {
			bool enabled = component.modules & moduleType;
			if (ImGui::Checkbox((std::string("Enable") + "##" + label).c_str(), &enabled)) {
				if (enabled) {
					component.modules |= moduleType;
					system.AddModule<T>(entity);
				}
				else {
					component.modules &= ~moduleType;
					system.RemoveModule<T>(entity);
				}

				dirty |= true;
			}

			if (enabled) {
				dirty |= drawFunc(*system.GetModule<T>(entity));
			}
		}

		ImGui::EndChild();

		return dirty;
	}

	bool ParticleComponentDrawer::Draw(Entity& entity) {
		auto& component = entity.GetComponent<ParticleComponent>();

		bool dirty = false;

		ImGui::Text("MaxParticles");
		ImGui::SameLine();

		if (ImGui::DragInt("##MaxParticles", &component.maxParticles, 1, 0, 10000)) {
			dirty |= true;
		}

		ImGui::Text("SpaceType");
		ImGui::SameLine();

		std::vector<std::string> spaceTypeEnumNames = { "Local", "World" };

		int32_t spaceType = (int32_t)component.spaceType;
		if (EditorHelper::DrawCombo("##SpaceType", spaceType, spaceTypeEnumNames)) {
			component.spaceType = (ParticleComponent::SpaceType)spaceType;
			dirty |= true;
		}

		ImGui::Text("StartSpeed");
		ImGui::SameLine();

		if (ImGui::DragFloat("##StartSpeed", &component.startSpeed, 0.1f)) {
			dirty |= true;
		}

		ImGui::Text("StartLifeTime");
		ImGui::SameLine();

		if (ImGui::DragFloat("##StartLifeTime", &component.startLifeTime, 0.1f)) {
			dirty |= true;
		}

		ImGui::Text("StartColor");
		ImGui::SameLine();

		if (ImGui::ColorEdit4("##startColor", glm::value_ptr(component.startColor))) {
			dirty |= true;
		}

		ImGui::Text("StartSize");
		ImGui::SameLine();

		if (ImGui::DragFloat3("##StartSize", glm::value_ptr(component.startSize), 0.1f)) {
			dirty |= true;
		}

		auto& particleSystem = entity.GetScene().GetParticleSystem();

		dirty |= DrawModule<EmissionModule>("Emission", particleSystem, entity, component, ParticleComponent::ModuleType::Emission, std::bind(&ParticleComponentDrawer::DrawEmissionModule, std::placeholders::_1));
		dirty |= DrawModule<ShapeModule>("Shape", particleSystem, entity, component, ParticleComponent::ModuleType::Shape, std::bind(&ParticleComponentDrawer::DrawShapeModule, std::placeholders::_1));
		dirty |= DrawModule<RandomSpeedModule>("RandomSpeed", particleSystem, entity, component, ParticleComponent::ModuleType::RandomSpeed, std::bind(&ParticleComponentDrawer::DrawRandomSpeedModule, std::placeholders::_1));
		dirty |= DrawModule<RandomColorModule>("RandomColor", particleSystem, entity, component, ParticleComponent::ModuleType::RandomColor, std::bind(&ParticleComponentDrawer::DrawRandomColorModule, std::placeholders::_1));
		dirty |= DrawModule<RandomSizeModule>("RandomSize", particleSystem, entity, component, ParticleComponent::ModuleType::RandomSize, std::bind(&ParticleComponentDrawer::DrawRandomSizeModule, std::placeholders::_1));
		dirty |= DrawModule<ColorOverLifetimeModule>("ColorOverLifetime", particleSystem, entity, component, ParticleComponent::ModuleType::ColorOverLifetime, std::bind(&ParticleComponentDrawer::DrawColorOverLifetimeModule, std::placeholders::_1));
		dirty |= DrawModule<SizeOverLifetimeModule>("SizeOverLifetime", particleSystem, entity, component, ParticleComponent::ModuleType::SizeOverLifetime, std::bind(&ParticleComponentDrawer::DrawSizeOverLifetimeModule, std::placeholders::_1));
		dirty |= DrawModule<NoiseModule>("Noise", particleSystem, entity, component, ParticleComponent::ModuleType::Noise, std::bind(&ParticleComponentDrawer::DrawNoiseModule, std::placeholders::_1));
		dirty |= DrawModule<RendererModule>("Renderer", particleSystem, entity, component, ParticleComponent::ModuleType::Renderer, std::bind(&ParticleComponentDrawer::DrawRendererModule, std::placeholders::_1));

		return dirty;
	}

	bool ParticleComponentDrawer::DrawEmissionModule(EmissionModule& module) {
		bool dirty = false;

		ImGui::Text("SpawnOverTime");
		ImGui::SameLine();
		if (ImGui::DragInt("##SpawnOverTime", &module.spawnOverTime, 1, 0, 10000)) {
			dirty |= true;
		}

		ImGui::Text("Burst");
		ImGui::SameLine();
		if (ImGui::Checkbox("##Burst", &module.burst)) {
			dirty |= true;
		}

		if (module.burst) {
			ImGui::Text("BurstStartTime");
			ImGui::SameLine();
			if (ImGui::DragFloat("##BurstStartTime", &module.burstStartTime, 0.1f)) {
				dirty |= true;
			}

			ImGui::Text("BurstParticleCount");
			ImGui::SameLine();
			if (ImGui::DragInt("##BurstParticleCount", reinterpret_cast<int*>(&module.burstParticleCount), 1, 0, 10000)) {
				dirty |= true;
			}

			ImGui::Text("BurstCycleCount");
			ImGui::SameLine();
			if (ImGui::DragInt("##BurstCycleCount", reinterpret_cast<int*>(&module.burstCycleCount), 1, 0, 10000)) {
				dirty |= true;
			}

			ImGui::Text("BurstCycleInterval");
			ImGui::SameLine();
			if (ImGui::DragFloat("##BurstCycleInterval", &module.burstCycleInterval, 0.1f)) {
				dirty |= true;
			}
		}

		return dirty;
	}

	bool ParticleComponentDrawer::DrawShapeModule(ShapeModule& module) {
		bool dirty = false;

		ImGui::Text("ShapeType");
		ImGui::SameLine();

		std::vector<std::string> shapeTypeEnumNames = { "None", "Sphere", "Box" };

		int32_t shapeType = (int32_t)module.shapeType;
		if (EditorHelper::DrawCombo("##ShapeType", shapeType, shapeTypeEnumNames)) {
			module.shapeType = (ShapeModule::ShapeType)shapeType;
			dirty |= true;
		}

		if (module.shapeType == ShapeModule::ShapeType::Sphere) {
			ImGui::Text("Radius");
			ImGui::SameLine();
			if (ImGui::DragFloat("##Radius", &module.sphere.radius, 0.1f)) {
				dirty |= true;
			}
			ImGui::Text("Thickness");
			ImGui::SameLine();
			if (ImGui::DragFloat("##Thickness", &module.sphere.thickness, 0.1f)) {
				dirty |= true;
			}
		}
		else if (module.shapeType == ShapeModule::ShapeType::Box) {
			ImGui::Text("Size");
			ImGui::SameLine();
			if (ImGui::DragFloat3("##Size", &module.box.size.x, 0.1f)) {
				dirty |= true;
			}
		}

		return dirty;
	}

	bool ParticleComponentDrawer::DrawRandomSpeedModule(RandomSpeedModule& module) {
		bool dirty = false;

		ImGui::Text("MinSpeed");
		ImGui::SameLine();
		if (ImGui::DragFloat("##MinSpeed", &module.minSpeed, 0.1f)) {
			dirty |= true;
		}

		ImGui::Text("MaxSpeed");
		ImGui::SameLine();
		if (ImGui::DragFloat("##MaxSpeed", &module.maxSpeed, 0.1f)) {
			dirty |= true;
		}

		return dirty;
	}

	bool ParticleComponentDrawer::DrawRandomColorModule(RandomColorModule& module) {
		bool dirty = false;

		ImGui::Text("MinColor");
		ImGui::SameLine();
		if (ImGui::ColorEdit4("##MinColor", glm::value_ptr(module.minColor))) {
			dirty |= true;
		}

		ImGui::Text("MaxColor");
		ImGui::SameLine();
		if (ImGui::ColorEdit4("##MaxColor", glm::value_ptr(module.maxColor))) {
			dirty |= true;
		}

		return dirty;
	}

	bool ParticleComponentDrawer::DrawRandomSizeModule(RandomSizeModule& module) {
		bool dirty = false;

		ImGui::Text("MinSize");
		ImGui::SameLine();
		if (ImGui::DragFloat3("##MinSize", &module.minSize.x, 0.1f)) {
			dirty |= true;
		}

		ImGui::Text("MaxSize");
		ImGui::SameLine();
		if (ImGui::DragFloat3("##MaxSize", &module.maxSize.x, 0.1f)) {
			dirty |= true;
		}

		return dirty;
	}

	bool ParticleComponentDrawer::DrawColorOverLifetimeModule(ColorOverLifetimeModule& module) {
		bool dirty = false;

		ImGui::Text("ColorOverLifetime");
		ImGui::SameLine();

		std::vector<std::string> easingEnumNames = { "Linear" };
		
		int32_t easing = (int32_t)module.easing;
		if (EditorHelper::DrawCombo("##Easing", easing, easingEnumNames)) {
			module.easing = (Easing)easing;
			dirty |= true;
		}

		ImGui::Text("EasingStartRatio");
		ImGui::SameLine();
		if (ImGui::DragFloat("##EasingStartRatio", &module.easingStartRatio, 0.1f)) {
			dirty |= true;
		}

		ImGui::Text("RedFactorRange");
		ImGui::SameLine();
		if (ImGui::DragFloat2("##RedFactorRange", &module.redFactorRange.x, 0.1f)) {
			dirty |= true;
		}

		ImGui::Text("GreenFactorRange");
		ImGui::SameLine();
		if (ImGui::DragFloat2("##GreenFactorRange", &module.greenFactorRange.x, 0.1f)) {
			dirty |= true;
		}

		ImGui::Text("BlueFactorRange");
		ImGui::SameLine();
		if (ImGui::DragFloat2("##BlueFactorRange", &module.blueFactorRange.x, 0.1f)) {
			dirty |= true;
		}

		ImGui::Text("AlphaFactorRange");
		ImGui::SameLine();
		if (ImGui::DragFloat2("##AlphaFactorRange", &module.alphaFactorRange.x, 0.1f)) {
			dirty |= true;
		}

		return dirty;
	}

	bool ParticleComponentDrawer::DrawSizeOverLifetimeModule(SizeOverLifetimeModule& module) {
		bool dirty = false;

		ImGui::Text("Easing");
		ImGui::SameLine();

		std::vector<std::string> easingEnumNames = { "Linear" };

		int32_t easing = (int32_t)module.easing;
		if (EditorHelper::DrawCombo("##Easing", easing, easingEnumNames)) {
			module.easing = (Easing)easing;
			dirty |= true;
		}

		ImGui::Text("EasingStartRatio");
		ImGui::SameLine();
		if (ImGui::DragFloat("##EasingStartRatio", &module.easingStartRatio, 0.1f)) {
			dirty |= true;
		}

		ImGui::Text("SizeFactorRange");
		ImGui::SameLine();
		if (ImGui::DragFloat2("##SizeFactorRange", &module.sizeFactorRange.x, 0.1f)) {
			dirty |= true;
		}

		return dirty;
	}

	bool ParticleComponentDrawer::DrawNoiseModule(NoiseModule& module) {
		bool dirty = false;

		ImGui::Text("Strength");
		ImGui::SameLine();
		if (ImGui::DragFloat("##Strength", &module.strength, 0.1f)) {
			dirty |= true;
		}

		ImGui::Text("Frequency");
		ImGui::SameLine();
		if (ImGui::DragFloat("##Frequency", &module.frequency, 0.1f)) {
			dirty |= true;
		}

		return dirty;
	}

	bool ParticleComponentDrawer::DrawRendererModule(RendererModule& module) {
		bool dirty = false;

		ImGui::Text("Alignment");
		ImGui::SameLine();

		std::vector<std::string> alignmentEnumNames = { "View", "Velocity" };

		int32_t alignment = (int32_t)module.alignment;
		if (EditorHelper::DrawCombo("##Alignment", alignment, alignmentEnumNames)) {
			module.alignment = (RendererModule::Alignment)alignment;
			dirty |= true;
		}

		return dirty;
	}
}
