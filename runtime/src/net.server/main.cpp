#include "net.common/log.h"
#include "net.common/time.h"
#include "net.core/tcp.h"

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
		tcp.async_write();
	}

	while (1)
	{
		timer.update();

		// 네트워크 루프
		if (tcp.get_state() == net::core::tcp::state::running)
		{
			// 디스패치
			size_t size = tcp.get_recv_queue().unsafe_size();
			for (int i = 0; i < size; i++)
			{
				net::core::packet_recv_context recv_context;
				if (tcp.get_recv_queue().try_pop(recv_context)) recv_context.invoke();
			}			
		}

		// 임시 busy wait 방지
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	return 0;
}
