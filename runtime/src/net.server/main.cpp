#include "net.core/tcp_server.h"
#include "net.common/log.h"
#include "net.common/time.h"
#include <chrono>
#include <thread>
#define TCP_PORT 60000

int main()
{
	net::common::log& logger = net::common::log::get_instance();
	{
		logger.init();
	}

	net::common::time& timer = net::common::time::get_instance();
	{
		timer.update();
	}

	net::core::tcp_server& tcp_server = net::core::tcp_server::get_instance();
	{
		tcp_server.init(TCP_PORT);
		tcp_server.async_accept();
	}

	while (tcp_server.get_state() == net::core::tcp_server::state::running)
	{
		timer.update();
	}

	return 0;
}
