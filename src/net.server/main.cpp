#include "pch.h"
#include "game_engine.h"
#include "net.core/tcp.h"
#include "net.logic/logic.h"
#include "net.common/log.h"
#include "net.common/time.h"

#define IP "192.168.0.3"
#define TCP_PORT 60000

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