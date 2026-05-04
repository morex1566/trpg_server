#include "tcp_server.h"

net::core::tcp_server::tcp_server() :
global_singleton(),
accept_token_bucket
(
	net::common::system_config::tcp_accept_token_bucket::capacity,
	net::common::system_config::tcp_accept_token_bucket::refill_interval_ms
)
{
	SPDLOG_INFO("create {} instance.", net::common::demangle(typeid(net::core::tcp_server).name()));
}

net::core::tcp_server::~tcp_server()
{
	close();

	// asio 디스패처 종료
	context.stop();
	work_guard.reset();
	delete_workers();
}

void net::core::tcp_server::init(boost::asio::ip::port_type port)
{
	endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);

	// asio 디스패처 시작
	context.restart();
	work_guard.emplace(boost::asio::make_work_guard(context));
	create_workers();
}

void net::core::tcp_server::async_accept()
{
	state expected = state::stopped;

	// 이미 tcp server 켜져 있었음
	if (!current_state.compare_exchange_strong(expected, state::running)) return;

	// accept 열기
	acceptor.emplace(context, endpoint);

	post_accept();
}

void net::core::tcp_server::create_workers()
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

void net::core::tcp_server::delete_workers()
{
	const auto current_thread_id = std::this_thread::get_id();

	for (auto& worker : context_workers)
	{
		if (!worker.joinable()) continue;

		if (worker.get_id() == current_thread_id)
		{
			worker.detach();
			continue;
		}

		worker.join();
	}

	context_workers.clear();
}

void net::core::tcp_server::post_accept()
{
	if (current_state.load() != state::running) return;
	if (!acceptor.has_value() || !acceptor->is_open()) return;

	acceptor->async_accept(
	[this](boost::system::error_code error, boost::asio::ip::tcp::socket client_socket)
	{
		// Socket 에러?
		if (error)
		{
			SPDLOG_WARN("socket error : {}.", error.message());
			if (current_state.load() == state::running) post_accept();
			return;
		}

		// TCP 멈춤?
		if (current_state.load() != state::running)
		{
			SPDLOG_INFO("tcp accept stopped. drop connection : {}", get_remote_address(client_socket));
			boost::system::error_code close_error;
			client_socket.close(close_error);
			return;
		}

		// Rate Limit 걸림?
		if (!accept_token_bucket.consume())
		{
			SPDLOG_INFO("server is busy. drop connection : {}", get_remote_address(client_socket));
			boost::system::error_code close_error;
			client_socket.close(close_error);
			post_accept();
			return;
		}

		// 연결을 받을 수 있을 만큼 메모리 충분?
		if (net::common::system_config::current_ram_percentage() > net::common::system_config::limit_ram_percentage)
		{
			SPDLOG_WARN("memory out. drop connection : {}", get_remote_address(client_socket));
			boost::system::error_code close_error;
			client_socket.close(close_error);
			post_accept();
			return;
		}

		// 커넥션 등록
		auto new_connection_id = net::common::connection_id_generator::generate();
		auto new_connection = std::make_shared<connection>(context, std::move(client_socket), new_connection_id);
		tbb::concurrent_hash_map<net::common::connection_id, std::shared_ptr<connection>>::accessor accessor;
		if (connections.insert(accessor, new_connection_id))
		{
			accessor->second = std::move(new_connection);
		}

		// 다음 연결 수락
		post_accept();
	});
}

void net::core::tcp_server::close()
{
	// 이미 stopped
	if (current_state.exchange(state::stopped) == state::stopped) return;

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

std::string net::core::tcp_server::get_remote_address(boost::asio::ip::tcp::socket& client_socket)
{
	boost::system::error_code error;
	const auto remote_endpoint = client_socket.remote_endpoint(error);

	if (error)
	{
		return "unknown";
	}

	return remote_endpoint.address().to_string();
}
