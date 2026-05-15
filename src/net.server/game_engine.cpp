#include "pch.h"
#include "game_engine.h"

game_engine::game_engine()
{
    SPDLOG_INFO("create {} instance.", typeid(game_engine).name());
}

game_engine::~game_engine()
{
}

std::shared_ptr<game_object> game_engine::instantiate()
{
    auto obj = std::make_shared<game_object>();
    pending_objects.push_back(obj);
    return obj;
}

void game_engine::dispatch_request()
{
    net::core::tcp& tcp = net::core::tcp::get_instance();
    auto& queue = tcp.get_requests();

    // tcp에 쌓인 request 처리
    int processed_count = 0;
    net::packet::packet_request request;
    while (processed_count < net::packet::packet_dispatcher::max_packet_dispatch_threshold && queue.try_dequeue(request))
    {
        try
        {
            // const packet_request* 타입으로 전달
            net::packet::packet_dispatcher::dispatch(&request);
        }
        catch (const std::exception& e)
        {
            SPDLOG_ERROR("dispatch error: session_id: {}, msg: {}", request.session_id, e.what());
        }

        processed_count++;
    }
}

void game_engine::dispatch_response()
{
}

void game_engine::init()
{
    is_running = true;
}

void game_engine::update(float dt)
{
    dispatch_request();

    for (auto& obj : game_objects)
    {
        if (!obj->active_self) continue;
        for (auto& comp : obj->get_components())
        {
            if (comp->enabled)
            {
                comp->update(dt);
            }
        }
    }
}