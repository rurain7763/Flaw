#pragma once

#include "Core.h"
#include "MonoScriptTypes.h"

#include <vector>

namespace flaw {
	class MonoScriptClass;
	class MonoScriptClassField;

	struct MonoScriptObjectTreeNode {
		std::string name;
		std::string typeName;

		virtual ~MonoScriptObjectTreeNode() = default;
	};

	struct MonoScriptObjectTreeNodeObject : public MonoScriptObjectTreeNode {
		std::vector<Ref<MonoScriptObjectTreeNode>> elements;
	};

	struct MonoScriptObjectTreeNodeValue : public MonoScriptObjectTreeNode {
		std::vector<int8_t> valueData;
	};

	class MonoScriptObjectView {
	public:
		MonoScriptObjectView(MonoScriptClass& clss, MonoObject* obj);

		MonoScriptClass GetClass() const;
		MonoObject* GetMonoObject() const;

		explicit operator bool() const;

	private:
		MonoDomain* _domain;
		MonoClass* _clss;
		MonoObject* _obj;
	};

	class MonoScriptObject {
	public:
		MonoScriptObject();
		MonoScriptObject(MonoScriptClass& clss);
		MonoScriptObject(MonoScriptObject&& other) noexcept;
		MonoScriptObject(const MonoScriptObject&) = delete;

		~MonoScriptObject();

		MonoScriptObject& operator=(MonoScriptObject&& other) noexcept;
		MonoScriptObject& operator=(const MonoScriptObject&) = delete;

		template <typename... Args>
		void Instantiate(Args&&... args) {
			if constexpr (sizeof...(args) == 0) {
				InstantiateDefaultConstructorImpl();
			}
			else {
				void* argsArray[] = { (void*)args... };
				InstantiateImpl(argsArray, sizeof...(args));
			}
		}

		template<typename... Args>
		void CallMethod(MonoMethod* method, Args&&... args) const {
			if constexpr (sizeof...(args) == 0) {
				CallMethodImpl(method);
			}
			else {
				void* argsArray[] = { (void*)args... };
				CallMethodImpl(method, argsArray, sizeof...(args));
			}
		}

		MonoScriptObject Clone(uint32_t fieldFlags = MonoFieldFlag_Public) const;

		Ref<MonoScriptObjectTreeNode> ToTree() const;
		void ApplyTree(const Ref<MonoScriptObjectTreeNode>& treeNode);

		MonoScriptClass GetClass() const;
		MonoScriptObjectView GetView() const;

		bool IsValid() const;

		MonoDomain* GetMonoDomain() const { return _domain; }
		MonoClass* GetMonoClass() const { return _clss; }
		MonoObject* GetMonoObject() const;

	private:
		void InstantiateDefaultConstructorImpl();
		void InstantiateImpl(void** constructorArgs, int32_t argCount);

		void CallMethodImpl(MonoMethod* method, void** args = nullptr, int32_t argCount = 0) const;

		void MonoScriptObject::DeepCloneFields(MonoObject* source, MonoObject* target, uint32_t expectedFieldsFlags, std::unordered_map<MonoObject*, MonoObject*>& clonedObjs) const;

		void CreateTreeNodeRecurcive(MonoObject* obj, Ref<MonoScriptObjectTreeNodeObject> node) const;
		void ApplyTreeNodeRecurcive(MonoObject* obj, Ref<MonoScriptObjectTreeNodeObject> node);

		void GcHandleNew(MonoObject* obj);
		void GcHandleFree();

	private:
		MonoDomain* _domain;
		MonoClass* _clss;

		uint32_t _gcHandle;
	};
}