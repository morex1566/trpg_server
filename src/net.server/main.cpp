#include "pch.h"
#include "net.core/tcp.h"
#include "net.logic/logic.h"
#include "net.common/log.h"
#include "net.common/time.h"
#include "game_engine.h"

#define IP "192.168.0.3"
#define TCP_PORT 60000
#define RUDP_PORT 50000
#define SESSION_SIZE 100000
#define SEND_BUFFER_SIZE 320LL * 1024 * 1024
#define RECV_BUFFER_SIZE 320LL * 1024 * 1024

int main()
{
	net::common::log& logger = net::common::log::get_instance();
	{
		logger.init();
	}

	net::common::time& timer = net::common::time::get_instance();
	{
		timer.tick();
	}

	net::core::tcp& tcp = net::core::tcp::get_instance();
	{
		tcp.init(TCP_PORT);
	}

	game_engine& engine = game_engine::get_instance();
	{
		engine.init();
	}

	tcp.start();

	while (engine.is_runnable() && tcp.is_runnable())
	{
		engine.update(timer.delta_time());
		timer.tick();
	}

	return 0;
}