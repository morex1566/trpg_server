#include "connection_id_generator.h"
#include <limits>

namespace net::common
{
    connection_id connection_id_generator::generate()
    {
        return last_connection_id.fetch_add(1, std::memory_order_relaxed) + 1;
    }
}
