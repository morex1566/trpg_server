#pragma once
#include "net.packet/packet.h"

namespace net::logic
{
    static void on_transform(const net::packet::packet_request* request, const net::protocol::transform* transform_in)
    {
        
    }
    REGISTER_HANDLER(net::protocol::packet_id_type_TRANSFORM, on_transform)
}