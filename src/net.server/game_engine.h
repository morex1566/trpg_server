#pragma once
#include "game_object.h"
#include "net.common/singleton.h"
#include "net.core/tcp.h"

class game_engine : public net::common::singleton<game_engine>
{

public:

    game_engine();
    ~game_engine() override;

    void init();

    // c2s
    void dispatch_request();

    // s2c
    void dispatch_response();

    void update(float dt);

    std::shared_ptr<game_object> instantiate();

    bool is_runnable() const { return is_running; }

private:

    bool is_running = false;
    std::vector<std::shared_ptr<game_object>> game_objects;
    std::vector<std::shared_ptr<game_object>> pending_objects;
};