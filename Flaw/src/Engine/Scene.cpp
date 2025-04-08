#include "pch.h"
#include "Scene.h"
#include "Components.h"
#include "Entity.h"
#include "Serialization.h"
#include "Physics/Physics2D.h"
#include "Time/Time.h"
#include "Platform/PlatformEvents.h"
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
		Entity entity(_registry.create(), this);
		entity.AddComponent<TransformComponent>();

		auto& enttComp = entity.AddComponent<EntityComponent>(name);
		enttComp.uuid.Generate();

		_entityMap[enttComp.uuid] = (entt::entity)(uint32_t)entity;

		return entity;
	}

	Entity Scene::CreateEntityByUUID(const UUID& uuid, const char* name) {
		Entity entity(_registry.create(), this);
		entity.AddComponent<TransformComponent>();

		auto& enttComp = entity.AddComponent<EntityComponent>(name);
		enttComp.uuid = uuid;

		_entityMap[enttComp.uuid] = (entt::entity)(uint32_t)entity;

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

	void Scene::DestroyEntityRecursive(Entity entity) {
		UUID entityUUID = entity.GetUUID();

		// call destroy to children
		entity.EachChildren([this](const Entity& child) { DestroyEntityRecursive(child); });
		_childMap.erase(entityUUID);

		if (entity.HasComponent<Rigidbody2DComponent>()) {
			auto& rigidbody2D = entity.GetComponent<Rigidbody2DComponent>();
			_physics2DWorld->DestroyBody((b2Body*)rigidbody2D.runtimeBody);
		}

		// TODO: mono script component를 가지고 있으면 instance를 삭제할 것

		_parentMap.erase(entityUUID);
		_entityMap.erase(entityUUID);

		_registry.destroy((entt::entity)(uint32_t)entity);
	}

	template <typename T>
	static void CopyComponentIfExists(Entity src, Entity dst) {
		if (src.HasComponent<T>()) {
			T* dstComp = nullptr;

			if (!dst.HasComponent<T>()) {
				dstComp = &dst.AddComponent<T>();
			}
			else {
				dstComp = &dst.GetComponent<T>();
			}

			*dstComp = src.GetComponent<T>();
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

		UUID copyUUID = cloned.GetUUID();

		CopyComponentIfExists<EntityComponent>(srcEntt, cloned);
		CopyComponentIfExists<TransformComponent>(srcEntt, cloned);
		CopyComponentIfExists<CameraComponent>(srcEntt, cloned);
		CopyComponentIfExists<SpriteRendererComponent>(srcEntt, cloned);
		CopyComponentIfExists<Rigidbody2DComponent>(srcEntt, cloned);
		CopyComponentIfExists<BoxCollider2DComponent>(srcEntt, cloned);
		CopyComponentIfExists<CircleCollider2DComponent>(srcEntt, cloned);
		CopyComponentIfExists<NativeScriptComponent>(srcEntt, cloned);
		CopyComponentIfExists<MonoScriptComponent>(srcEntt, cloned);
		CopyComponentIfExists<TextComponent>(srcEntt, cloned);
		CopyComponentIfExists<SoundListenerComponent>(srcEntt, cloned);
		CopyComponentIfExists<SoundSourceComponent>(srcEntt, cloned);

		// because uuid is changed, we need to set it again
		cloned.GetComponent<EntityComponent>().uuid = copyUUID;

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
				// TODO: 부모-자식 관계를 고려한 offset으로 변경할 것
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

		Scripting::OnStart(this);
	}

	void Scene::OnUpdate() {
		{
			// sound
			for (auto&& [listenerEntity, transComp, soundListenerComp] : _registry.view<TransformComponent, SoundListenerComponent>().each()) {
				SoundListener listener;
				listener.position = transComp.position;
				listener.velocity = soundListenerComp.velocity;
				listener.forward = transComp.GetFront();
				listener.up = transComp.GetUp();

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

		{
			// update mono scripts
			Scripting::OnUpdate();

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

		{
			// update physics
			const int32_t velocityIterations = 6;
			const int32_t positionIterations = 2;

			_physics2DWorld->Step(Time::DeltaTime(), velocityIterations, positionIterations);

			for (auto&& [entity, transform, rigidbody2D] : _registry.view<TransformComponent, Rigidbody2DComponent>().each()) {
				b2Body* body = (b2Body*)rigidbody2D.runtimeBody;

				if (body == nullptr) {
					continue;
				}

				transform.position = vec3(body->GetPosition().x, body->GetPosition().y, transform.position.z);
				transform.rotation.z = body->GetAngle();

				rigidbody2D.linearVelocity = vec2(body->GetLinearVelocity().x, body->GetLinearVelocity().y);
			}
		}

		{
			// render
			struct CameraMatrices {
				mat4 view;
				mat4 projection;
			};

			std::map<uint32_t, CameraMatrices> sortedCameras;
			for (auto&& [entity, transform, camera] : _registry.view<TransformComponent, CameraComponent>().each()) {
				sortedCameras.insert({ camera.depth, { ViewMatrix(transform.position, transform.rotation), camera.GetProjectionMatrix() } });
			}

			// draw entities
			for (const auto& [depth, matrices] : sortedCameras) {
				Renderer2D::Begin(matrices.view, matrices.projection);

				// draw sprite
				for (auto&& [entity, transform, sprite] : _registry.view<TransformComponent, SpriteRendererComponent>().each()) {
					auto textureAsset = AssetManager::GetAsset<Texture2DAsset>(sprite.texture);
					if (!textureAsset) {
						Renderer2D::DrawQuad((uint32_t)entity, transform.GetTransform(), sprite.color);
					}
					else {
						Renderer2D::DrawQuad((uint32_t)entity, transform.GetTransform(), textureAsset->GetTexture());
					}
				}

				// draw text
				for (auto&& [entity, transform, text] : _registry.view<TransformComponent, TextComponent>().each()) {
					auto fontAsset = AssetManager::GetAsset<FontAsset>(text.font);
					if (fontAsset) {
						Renderer2D::DrawString((uint32_t)entity, transform.GetTransform(), text.text, fontAsset->GetFont(), fontAsset->GetFontAtlas(), text.color);
					}
				}

				Renderer2D::End();
			}
		}
	}

	void Scene::OnEnd() {
		Scripting::OnEnd();

		_physics2DWorld.reset();
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