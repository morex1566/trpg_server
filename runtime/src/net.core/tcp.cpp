#include "tcp.h"
#include "net.common/connection_id_generator.h"
#include "net.common/guid_generator.h"
#include "net.common/time.h"
#include "net.core/service.h"

net::core::tcp::tcp() :
global_singleton(),
accept_token_bucket
(
	net::common::system_config::tcp_accept_token_bucket::capacity,
	net::common::system_config::tcp_accept_token_bucket::refill_interval_ms
)
{
	SPDLOG_INFO("create {} instance.", net::common::demangle(typeid(net::core::tcp).name()));
}

net::core::tcp::~tcp()
{
	close();

	// asio 디스패처 종료
	work_guard.reset();
	delete_workers();
	context.stop();
}

void net::core::tcp::init(boost::asio::ip::port_type port)
{
	endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);

	// asio 디스패처 시작
	context.restart();
	work_guard.emplace(boost::asio::make_work_guard(context));
	create_workers();
}

void net::core::tcp::async_accept()
{
	// 이미 tcp 켜져 있었음
	state expected = state::stopped;
	if (!current_state.compare_exchange_strong(expected, state::running)) return;

	// accept 열기
	acceptor.emplace(context, endpoint);

	post_accept();
}

void net::core::tcp::update()
{
	if (current_state.load() != state::running) return;

	tick_elapsed_ms += net::common::time::get_instance().deltatime(net::common::time::time_unit_type::millisecond);
	if (tick_elapsed_ms < net::common::system_config::tcp::tick_interval_ms) return;

	tick_elapsed_ms = 0.f;

	std::vector<uint64_t> disconnected_ids;
	for (auto iterator = connections.begin(); iterator != connections.end(); ++iterator)
	{
		if (!iterator->second || iterator->second->is_disconnected())
		{
			disconnected_ids.push_back(iterator->first);
			continue;
		}

		// tick마다 write 시작만 요청한다. 실제 send_queue 접근과 socket write는 connection strand에서 처리
		iterator->second->async_write();
	}

	for (const auto connection_id : disconnected_ids)
	{
		// 끊긴 connection은 map에서 제거해 장시간 운영 시 누적되지 않도록 함
		connections.erase(connection_id);
	}
}

void net::core::tcp::create_workers()
{
	context_workers.clear();

	const unsigned int hardware_thread_count = std::thread::hardware_concurrency();
	const unsigned int thread_count = hardware_thread_count > 1 ? hardware_thread_count - 1 : 1;
	for (unsigned int i = 0; i < thread_count; i++)
	{
		// 메인 스레드를 제외한 나머지 스레드를 asio 디스패치 처리에 등록
		context_workers.emplace_back([this]()
		{
			context.run();
		});
	}
}

void net::core::tcp::delete_workers()
{
	const auto current_thread_id = std::this_thread::get_id();

	for (auto& worker : context_workers)
	{
		if (!worker.joinable()) continue;

		// 함수 콜이 자기 자신이면 join하지 않고 detach
		if (worker.get_id() == current_thread_id)
		{
			worker.detach();
			continue;
		}

		worker.join();
	}

	context_workers.clear();
}

void net::core::tcp::post_accept()
{
	if (current_state.load() != state::running) return;

	acceptor->async_accept(
	[this](boost::system::error_code error, boost::asio::ip::tcp::socket client_socket)
	{
		// Socket 에러?
		if (error)
		{
			SPDLOG_WARN("socket error : {}.", error.message());
			if (current_state.load() == state::running)
			{
				// 일시적인 accept 실패로 서버가 멈추지 않도록 다음 accept를 이어감
				post_accept();
			}
			return;
		}

		// TCP 멈춤? 지금 들어온 클라 버림
		if (current_state.load() != state::running)
		{
			SPDLOG_INFO("tcp accept stopped. drop connection : {}", get_remote_address(client_socket));
			return;
		}

		// Rate Limit 걸림? 지금 들어온 클라 버림
		if (!accept_token_bucket.consume())
		{
			SPDLOG_INFO("server is busy. drop connection : {}", get_remote_address(client_socket));
			post_accept();
			return;
		}

		// 연결을 받을 수 있을 만큼 메모리 충분? 지금 들어온 클라 버림
		if (net::common::system_config::current_ram_percentage() > net::common::system_config::limit_ram_percentage)
		{
			SPDLOG_WARN("memory out. drop connection : {}", get_remote_address(client_socket));
			post_accept();
			return;
		}

		// 커넥션 등록
		auto new_connection_id = net::common::connection_id_generator::generate();
		auto new_connection_guid = net::common::guid_generator::generate();
		auto new_connection = std::make_shared<connection>(context, std::move(client_socket), new_connection_guid);
		tbb::concurrent_hash_map<uint64_t, std::shared_ptr<connection>>::accessor accessor;
		if (connections.insert(accessor, new_connection_id))
		{
			accessor->second = std::move(new_connection);
			accessor->second->async_read();
		}

		// 다음 연결 수락
		post_accept();
	});
}

void net::core::tcp::close()
{
	// connected 상태가 아님.
	state expected = state::running;
	if (!current_state.compare_exchange_strong(expected, state::stopped)) return;

	// 대기 중인 accept를 취소한 뒤 acceptor를 닫아 신규 접속을 막는다.
	if (acceptor.has_value() && acceptor->is_open())
	{
		boost::system::error_code error;
		acceptor->cancel(error);
		error.clear();
		acceptor->close(error);
	}
	acceptor.reset();

	// 클라들 연결 종료
	for (auto iterator = connections.begin(); iterator != connections.end(); ++iterator)
	{
		if (iterator->second)
		{
			iterator->second->close();
		}
	}
	connections.clear();
}

std::string net::core::tcp::get_remote_address(boost::asio::ip::tcp::socket& client_socket)
{
	boost::system::error_code error;
	const auto remote_endpoint = client_socket.remote_endpoint(error);

	if (error)
	{
		return "unknown";
	}

	return remote_endpoint.address().to_string();
}
