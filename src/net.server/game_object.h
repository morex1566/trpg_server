#pragma once
#include <vector>
#include <memory>
#include "game_component.h"

class game_object
{
public:

	//

protected:

	//

private:

	//

public:

	game_object() = default;
	~game_object() = default;

protected:

	//

private:

	//

public:

    // 컴포넌트 추가
    template<typename T, typename... Args>
    std::shared_ptr<T> add_component(Args&&... args)
    {
        auto comp = std::make_shared<T>(std::forward<Args>(args)...);
        comp->object = this;
        components.push_back(comp);
        return comp;
    }

    // 컴포넌트 가져오기
    template<typename T>
    std::shared_ptr<T> get_component()
    {
        for (auto& comp : components)
        {
            if (auto casted = std::dynamic_pointer_cast<T>(comp))
                return casted;
        }
        return nullptr;
    }

    void set_active(bool active)
    {
        active_self = active;
    }

    const std::vector<std::shared_ptr<game_component>>& get_components() const { return components; }

protected:

	//

private:

	//

public:

    uint32_t object_id = 0;
    bool active_self = true;

protected:

	//

private:

    std::vector<std::shared_ptr<game_component>> components;
};