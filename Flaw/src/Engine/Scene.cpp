#include "pch.h"
#include "Scene.h"
#include "Components.h"
#include "Entity.h"
#include "Serialization.h"
#include "Physics/Physics2D.h"
#include "Time/Time.h"
#include "Platform/PlatformEvents.h"
#include "ParticleSystem.h"
#include "RenderSystem.h"
#include "SkyBoxSystem.h"
#include "LandscapeSystem.h"
#include "ShadowSystem.h"
#include "AnimationSystem.h"
#include "MonoScriptSystem.h"
#include "PhysicsSystem.h"
#include "Scripting.h"
#include "Renderer2D.h"
#include "AssetManager.h"
#include "Assets.h"
#include "Sounds.h"
#include "Log/Log.h"

namespace flaw {
	Scene::Scene(Application& app) 
		: _app(app)
	{
		_particleSystem = CreateScope<ParticleSystem>(*this);
		_skyBoxSystem = CreateScope<SkyBoxSystem>(*this);
		_landscapeSystem = CreateScope<LandscapeSystem>(*this);		
		_shadowSystem = CreateScope<ShadowSystem>(*this);		
		_animationSystem = CreateScope<AnimationSystem>(_app, *this);
		_monoScriptSystem = CreateScope<MonoScriptSystem>(_app, *this);
		_physicsSystem = CreateScope<PhysicsSystem>(*this);
		_renderSystem = CreateScope<RenderSystem>(*this);

		_app.GetEventDispatcher().Register<WindowResizeEvent>([this](const WindowResizeEvent& evn) {
			for (auto&& [entity, camera] : _registry.view<CameraComponent>().each()) {
				camera.aspectRatio = (float)evn.width / (float)evn.height;
			}
		}, PID(this));
	}

	Scene::~Scene() {
		_app.GetEventDispatcher().Unregister<WindowResizeEvent>(PID(this));
	}

	Entity Scene::CreateEntity(const char* name) {
		return CreateEntity(vec3(0.f), vec3(0.f), vec3(1.f), name);
	}

	Entity Scene::CreateEntity(const vec3& position, const vec3& rotation, const vec3& scale, const char* name) {
		Entity entity(_registry.create(), this);

		UUID uuid;
		uuid.Generate();
		entity.AddComponent<EntityComponent>(uuid, name);

		entity.AddComponent<TransformComponent>(position, rotation, scale);

		_entityMap[uuid] = (entt::entity)entity;
		
		return entity;
	}

	Entity Scene::CreateEntityByUUID(const UUID& uuid, const char* name) {
		Entity entity(_registry.create(), this);
		entity.AddComponent<EntityComponent>(uuid, name);
		entity.AddComponent<TransformComponent>();

		_entityMap[uuid] = (entt::entity)entity;

		return entity;
	}

	void Scene::DestroyEntity(Entity entity) {
		UUID entityUUID = entity.GetUUID();

		auto it = _parentMap.find(entityUUID);
		if (it != _parentMap.end()) {
			_childMap[it->second].erase(entityUUID);
		}

		DestroyEntityRecursive(entity);
	}

	void Scene::DestroyEntityByUUID(const UUID& uuid) {
		auto it = _entityMap.find(uuid);
		if (it != _entityMap.end()) {
			Entity entity(it->second, this);
			DestroyEntity(entity);
		}
	}

	void Scene::DestroyEntityRecursive(Entity entity) {
		UUID entityUUID = entity.GetUUID();

		// call destroy to children
		entity.EachChildren([this](const Entity& child) { DestroyEntityRecursive(child); });
		_childMap.erase(entityUUID);

		if (_physics2DWorld && entity.HasComponent<Rigidbody2DComponent>()) {
			auto& rigidbody2D = entity.GetComponent<Rigidbody2DComponent>();
			_physics2DWorld->DestroyBody((b2Body*)rigidbody2D.runtimeBody);
		}

		// TODO: mono script component를 가지고 있으면 instance를 삭제할 것

		_parentMap.erase(entityUUID);
		_entityMap.erase(entityUUID);

		_registry.destroy((entt::entity)entity);
	}

	template <typename T>
	static void CloneComponent(Entity src, Entity dst) {
		if (!src.HasComponent<T>()) {
			return;
		}

		T& srcComp = src.GetComponent<T>();

		if constexpr (std::is_same_v<T, EntityComponent>) {
			dst.GetComponent<EntityComponent>().name = srcComp.name;
		}
		else if constexpr (std::is_same_v<T, TransformComponent>) {
			dst.GetComponent<TransformComponent>() = srcComp;
		}
		else {
			dst.AddComponent<T>(srcComp);
		}
	}

	Entity Scene::CloneEntity(const Entity& srcEntt, bool sameUUID) {
		Entity cloned;
		if (!sameUUID) {
			cloned = CreateEntity();
		}
		else {
			cloned = CreateEntityByUUID(srcEntt.GetUUID());
		}

		CloneComponent<EntityComponent>(srcEntt, cloned);
		CloneComponent<TransformComponent>(srcEntt, cloned);
		CloneComponent<CameraComponent>(srcEntt, cloned);
		CloneComponent<SpriteRendererComponent>(srcEntt, cloned);
		CloneComponent<Rigidbody2DComponent>(srcEntt, cloned);
		CloneComponent<BoxCollider2DComponent>(srcEntt, cloned);
		CloneComponent<CircleCollider2DComponent>(srcEntt, cloned);
		CloneComponent<RigidbodyComponent>(srcEntt, cloned);
		CloneComponent<BoxColliderComponent>(srcEntt, cloned);
		CloneComponent<SphereColliderComponent>(srcEntt, cloned);
		CloneComponent<MeshColliderComponent>(srcEntt, cloned);
		CloneComponent<NativeScriptComponent>(srcEntt, cloned);
		CloneComponent<TextComponent>(srcEntt, cloned);
		CloneComponent<SoundListenerComponent>(srcEntt, cloned);
		CloneComponent<SoundSourceComponent>(srcEntt, cloned);
		CloneComponent<ParticleComponent>(srcEntt, cloned);
		CloneComponent<StaticMeshComponent>(srcEntt, cloned);
		CloneComponent<SkeletalMeshComponent>(srcEntt, cloned);
		CloneComponent<SkyLightComponent>(srcEntt, cloned);
		CloneComponent<DirectionalLightComponent>(srcEntt, cloned);
		CloneComponent<PointLightComponent>(srcEntt, cloned);
		CloneComponent<SpotLightComponent>(srcEntt, cloned);
		CloneComponent<SkyBoxComponent>(srcEntt, cloned);
		CloneComponent<DecalComponent>(srcEntt, cloned);
		CloneComponent<LandscapeComponent>(srcEntt, cloned);
		CloneComponent<AnimatorComponent>(srcEntt, cloned);
		CloneComponent<MonoScriptComponent>(srcEntt, cloned);

		// clone children
		srcEntt.EachChildren([this, &cloned, sameUUID](const Entity& child) {
			Entity clonedChild = CloneEntity(child, sameUUID);
			clonedChild.SetParent(cloned);
		});

		return cloned;
	}

	Entity Scene::FindEntityByName(const char* name) {
		for (auto&& [entity, entityComp] : _registry.view<EntityComponent>().each()) {
			if (entityComp.name == name) {
				return Entity(entity, this);
			}
		}
		return Entity();
	}

	Entity Scene::FindEntityByUUID(const UUID& uuid) {
		auto it = _entityMap.find(uuid);
		if (it != _entityMap.end()) {
			return Entity(it->second, this);
		}
		return Entity();
	}

	static b2BodyType RigidBody2DBTypeToBox2DBType(Rigidbody2DComponent::BodyType type) {
		switch (type) {
			case Rigidbody2DComponent::BodyType::Static: return b2_staticBody;
			case Rigidbody2DComponent::BodyType::Dynamic: return b2_dynamicBody;
			case Rigidbody2DComponent::BodyType::Kinematic: return b2_kinematicBody;
		}
		
		throw std::runtime_error("Unknown RigidBody2DComponent::BodyType");
	}

	void Scene::OnStart() {
		_physics2DWorld = CreateScope<b2World>(b2Vec2(0.0f, -9.8f));

		for (auto&& [entity, transComp, rigidbody2DComp] : _registry.view<TransformComponent, Rigidbody2DComponent>().each()) {
			Entity entt(entity, this);

			// TODO: 다른 콜라이더 추가 시 수정할 것
			bool hasAnyCollider = entt.HasComponent<BoxCollider2DComponent>() || entt.HasComponent<CircleCollider2DComponent>();
			if (!hasAnyCollider) {
				continue;
			}

			b2BodyDef bodyDef;
			bodyDef.type = RigidBody2DBTypeToBox2DBType(rigidbody2DComp.bodyType);
			// TODO: consider parent 
			bodyDef.position.Set(transComp.position.x, transComp.position.y);
			bodyDef.angle = transComp.rotation.z;

			b2Body* runtimeBody = _physics2DWorld->CreateBody(&bodyDef);
			runtimeBody->SetFixedRotation(rigidbody2DComp.fixedRotation);

			rigidbody2DComp.runtimeBody = runtimeBody;

			if (entt.HasComponent<BoxCollider2DComponent>()) {
				auto& boxCollider = entt.GetComponent<BoxCollider2DComponent>();

				b2PolygonShape shape;
				// TODO: 부모-자식 관계를 고려한 사이즈로 변경할 것
				shape.SetAsBox(boxCollider.size.x * transComp.scale.x, boxCollider.size.y * transComp.scale.y, b2Vec2(boxCollider.offset.x, boxCollider.offset.y), 0.0f);

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &shape;
				fixtureDef.density = rigidbody2DComp.density;
				fixtureDef.friction = rigidbody2DComp.friction;
				fixtureDef.restitution = rigidbody2DComp.restitution;
				fixtureDef.restitutionThreshold = rigidbody2DComp.restitutionThreshold;

				runtimeBody->CreateFixture(&fixtureDef);
			}
			else if (entt.HasComponent<CircleCollider2DComponent>()) {
				auto& circleCollider = entt.GetComponent<CircleCollider2DComponent>();

				b2CircleShape shape;
				// TODO: 부모-자식 관계를 고려한 offset 변경할 것
				shape.m_p = b2Vec2(circleCollider.offset.x, circleCollider.offset.y);
				shape.m_radius = circleCollider.radius;

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &shape;
				fixtureDef.density = rigidbody2DComp.density;
				fixtureDef.friction = rigidbody2DComp.friction;
				fixtureDef.restitution = rigidbody2DComp.restitution;
				fixtureDef.restitutionThreshold = rigidbody2DComp.restitutionThreshold;

				runtimeBody->CreateFixture(&fixtureDef);
			}
		}

		_physicsSystem->Start();
		_animationSystem->Start();
		_monoScriptSystem->Start();
	}

	void Scene::OnUpdate() {
		_monoScriptSystem->Update();
		UpdateScript();
		_animationSystem->Update();
		UpdatePhysics2D();
		_physicsSystem->Update();
		UpdateSound();
		UpdateTransform();
		_renderSystem->Update();

		_renderSystem->Render();
	}

	void Scene::OnEnd() {
		_monoScriptSystem->End();
		_animationSystem->End();
		_physicsSystem->End();
		_physics2DWorld.reset();
	}

	void Scene::UpdateSound() {
		for (auto&& [listenerEntity, transComp, soundListenerComp] : _registry.view<TransformComponent, SoundListenerComponent>().each()) {
			SoundListener listener;
			listener.position = transComp.position;
			listener.velocity = soundListenerComp.velocity;
			listener.forward = transComp.GetWorldFront();
			listener.up = transComp.GetWorldUp();

			Sounds::SetListener(listener);

			for (auto&& [soundSrcEntity, transComp, soundSrcComp] : _registry.view<TransformComponent, SoundSourceComponent>().each()) {
				auto soundAsset = AssetManager::GetAsset<SoundAsset>(soundSrcComp.sound);
				if (soundAsset) {
					auto soundSrc = soundAsset->GetSoundSource();

					if ((!soundSrcComp.channel && soundSrcComp.autoPlay) || soundSrcComp.signal == SoundSourceComponent::Signal::Play) {
						soundSrcComp.channel = soundSrc->Play(soundSrcComp.loop ? -1 : 0);
					}

					if (soundSrcComp.channel) {
						if (soundSrcComp.signal == SoundSourceComponent::Signal::Resume) {
							soundSrcComp.channel->Resume();
						}
						else if (soundSrcComp.signal == SoundSourceComponent::Signal::Stop) {
							soundSrcComp.channel->Stop();
						}

						soundSrcComp.channel->SetPosition3D(transComp.position);
						soundSrcComp.channel->SetVolume(soundSrcComp.volume);
					}

					soundSrcComp.signal = SoundSourceComponent::Signal::None;
				}
			}

			// listener must be exist only one
			break;
		}
	}

	void Scene::UpdateScript() {
		// update native scripts
		for (auto&& [entity, scriptable] : _registry.view<NativeScriptComponent>().each()) {
			if (scriptable.instance == nullptr) {
				scriptable.instance = scriptable.instantiateFunc();
				scriptable.instance->_entity = Entity(entity, this);
				scriptable.instance->OnCreate();
			}
			scriptable.instance->OnUpdate();
		}
	}

	void Scene::UpdatePhysics2D() {
		const int32_t velocityIterations = 6;
		const int32_t positionIterations = 2;

		_physics2DWorld->Step(Time::DeltaTime(), velocityIterations, positionIterations);

		for (auto&& [entity, transform, rigidbody2D] : _registry.view<TransformComponent, Rigidbody2DComponent>().each()) {
			b2Body* body = (b2Body*)rigidbody2D.runtimeBody;

			if (body == nullptr) {
				continue;
			}

			// TODO: 부모-자식 관계일때는 어떻게 할 것인가?
			transform.dirty = true;
			transform.position = vec3(body->GetPosition().x, body->GetPosition().y, transform.position.z);
			transform.rotation.z = body->GetAngle();

			rigidbody2D.linearVelocity = vec2(body->GetLinearVelocity().x, body->GetLinearVelocity().y);
		}
	}

	static void CalculateWorldTransformRecursive(const mat4& parentTransform, Entity entity, bool calculateAnyway = false) {
		TransformComponent& transform = entity.GetComponent<TransformComponent>();

		bool shouldCalculate = calculateAnyway || transform.dirty;

		// 항상 parent 기준으로 초기화
		if (shouldCalculate) {
			transform.worldTransform = parentTransform * ModelMatrix(transform.position, transform.rotation, transform.scale);
			transform.dirty = false;
		}

		// 자식들에게 재귀적으로 전파
		entity.EachChildren([&worldTransform = transform.worldTransform, shouldCalculate](const Entity& child) {
			CalculateWorldTransformRecursive(worldTransform, child, shouldCalculate);
		});
	}

	void Scene::UpdateTransform() {
		for (auto&& [entity, transform] : _registry.view<TransformComponent>().each()) {
			Entity entt(entity, this);

			if (entt.HasParent()) {
				continue;
			}

			CalculateWorldTransformRecursive(mat4(1.0f), entt);
		}
	}

	void Scene::UpdateTransformImmediate(entt::entity entity) {
		Entity entt(entity, this);
		
		while (entt.HasParent()) {
			entt = entt.GetParent();
		}

		CalculateWorldTransformRecursive(mat4(1.0f), entt, true);
	}

	void Scene::ToFile(const char* filepath) {
		std::ofstream file(filepath);
		YAML::Emitter out;
		Serialize(out, *this);
		file << out.c_str();
		file.close();
	}

	void Scene::FromFile(const char* filepath) {
		YAML::Node node = YAML::LoadFile(filepath);
		if (!node) {
			Log::Error("Failed to load file %s", filepath);
			return;
		}
		Deserialize(node, *this);
	}

	Ref<Scene> Scene::Clone() {
		Ref<Scene> scene = CreateRef<Scene>(_app);

		// clone entities
		for (auto&& [entity] : _registry.view<entt::entity>().each()) {
			Entity srcEntity(entity, this);

			if (srcEntity.HasParent()) {
				continue;
			}

			scene->CloneEntity(srcEntity, true);
		}

		scene->_parentMap = _parentMap;
		scene->_childMap = _childMap;

		return scene;
	}
}