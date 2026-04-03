#pragma once
#include "packet.h"

namespace net::core
{
    static void on_transform(const packet_request* request, const net::protocol::transform* transform_in)
    {
        
    }
    REGISTER_HANDLER(net::protocol::packet_id_type_TRANSFORM, on_transform)
}