#include "net.core/tcp.h"
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

	net::core::tcp& tcp = net::core::tcp::get_instance();
	{
		tcp.init(TCP_PORT);
		tcp.async_accept();
	}

	while (tcp.get_state() == net::core::tcp::state::running)
	{
		timer.update();
	}

	return 0;
}
