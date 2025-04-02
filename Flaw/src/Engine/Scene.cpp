#include "pch.h"
#include "Scene.h"
#include "Components.h"
#include "Entity.h"
#include "Serialization.h"
#include "Physics/Physics2D.h"
#include "Time/Time.h"
#include "Platform/PlatformEvents.h"
#include "Scripting.h"

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
		entity.AddComponent<EntityComponent>(name);
		entity.AddComponent<TransformComponent>();

		return entity;
	}

	void Scene::DestroyEntity(Entity entity) {
		if (entity.HasComponent<Rigidbody2DComponent>()) {
			auto& rigidbody2D = entity.GetComponent<Rigidbody2DComponent>();
			_physics2DWorld->DestroyBody((b2Body*)rigidbody2D.runtimeBody);
		}

		// TODO: mono script component를 가지고 있으면 instance를 삭제할 것

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

	Entity Scene::CloneEntity(const Entity& srcEntt) {
		Entity dstEntt = CreateEntity();

		// TODO: this is not efficient, so we need reflection system
		// clone components
		CopyComponentIfExists<EntityComponent>(srcEntt, dstEntt);
		CopyComponentIfExists<TransformComponent>(srcEntt, dstEntt);
		CopyComponentIfExists<CameraComponent>(srcEntt, dstEntt);
		CopyComponentIfExists<SpriteRendererComponent>(srcEntt, dstEntt);
		CopyComponentIfExists<Rigidbody2DComponent>(srcEntt, dstEntt);
		CopyComponentIfExists<BoxCollider2DComponent>(srcEntt, dstEntt);
		CopyComponentIfExists<CircleCollider2DComponent>(srcEntt, dstEntt);
		CopyComponentIfExists<NativeScriptComponent>(srcEntt, dstEntt);
		CopyComponentIfExists<MonoScriptComponent>(srcEntt, dstEntt);

		return dstEntt;
	}

	Entity Scene::FindEntityByName(const char* name) {
		for (auto&& [entity, entityComp] : _registry.view<EntityComponent>().each()) {
			if (entityComp.name == name) {
				return Entity(entity, this);
			}
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
			bodyDef.position.Set(transComp.position.x, transComp.position.y);
			bodyDef.angle = transComp.rotation.z;

			b2Body* runtimeBody = _physics2DWorld->CreateBody(&bodyDef);
			runtimeBody->SetFixedRotation(rigidbody2DComp.fixedRotation);

			rigidbody2DComp.runtimeBody = runtimeBody;

			if (entt.HasComponent<BoxCollider2DComponent>()) {
				auto& boxCollider = entt.GetComponent<BoxCollider2DComponent>();

				b2PolygonShape shape;
				// TODO: 부모-자식 관계를 고려한 사이즈로 변경할 것
				shape.SetAsBox(boxCollider.size.x * transComp.scale.x, boxCollider.size.y * transComp.scale.y);

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

			auto& renderer2D = _app.GetRenderer2D();

			// draw entities
			for (const auto& [depth, matrices] : sortedCameras) {
				renderer2D.Begin(matrices.view, matrices.projection);

				// draw sprite
				for (auto&& [entity, transform, sprite] : _registry.view<TransformComponent, SpriteRendererComponent>().each()) {
					if (sprite.texture == nullptr) {
						renderer2D.DrawQuad((uint32_t)entity, transform.GetTransform(), sprite.color);
					}
					else {
						renderer2D.DrawQuad((uint32_t)entity, transform.GetTransform(), sprite.texture);
					}
				}

				renderer2D.End();
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
		for (auto&& [entity, srdEnttComp] : _registry.view<entt::entity, EntityComponent>().each()) {
			Entity cloned = scene->CloneEntity(Entity(entity, this));

			// set uuid same as source entity
			cloned.GetComponent<EntityComponent>().uuid = srdEnttComp.uuid;
		}
		return scene;
	}
}