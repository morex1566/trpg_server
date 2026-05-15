#pragma once
class game_object;
class transform;

class game_component
{
public:

	//

protected:

	//

private:

	//

public:

    virtual ~game_component() = default;

protected:

	//

private:

	//

public:

    virtual void awake() {}
    virtual void on_enable() {}
    virtual void start() {}
    virtual void fixed_update(float fixed_dt) {}
    virtual void update(float dt) {}
    virtual void late_update() {}
    virtual void on_disable() {}
    virtual void on_destroy() {}

protected:

	//

private:

	//

public:

    game_object* object = nullptr;
    transform* transform_ptr = nullptr;
    bool enabled = true;

protected:

	//

private:

	//
};
