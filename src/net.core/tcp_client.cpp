#include "tcp_client.h"

net::core::tcp_client::tcp_client() :
global_singleton(),
resolver(context),
socket(context)
{
	SPDLOG_INFO("create {} instance.", net::common::demangle(typeid(net::core::tcp_client).name()));
}

net::core::tcp_client::~tcp_client()
{
	close();

	// asio 디스패칭 종료
	context.stop();
	work_guard.reset();
	delete_workers();
}

void net::core::tcp_client::init(const std::string& host, boost::asio::ip::port_type port)
{
	this->host = host;
	this->port = port;

	// asio 디스패칭 시작
	context.restart();
	work_guard.emplace(boost::asio::make_work_guard(context));
	create_workers();
}

void net::core::tcp_client::async_connect()
{
	state expected = state::disconnected;

	// 이미 tcp client 연결 중이거나 연결되어 있었음
	if (!current_state.compare_exchange_strong(expected, state::connecting)) return;
	
	// 클라에 연결할 소켓
	socket = boost::asio::ip::tcp::socket(context);

	post_resolve();
}

void net::core::tcp_client::post_resolve()
{
	if (current_state.load() != state::connecting) return;

	resolver.async_resolve(host, std::to_string(port),
	[this](boost::system::error_code error, boost::asio::ip::tcp::resolver::results_type results)
	{
		// Resolve 에러?
		if (error)
		{
			SPDLOG_WARN("resolve error : {}.", error.message());
			current_state.store(state::disconnected);
			return;
		}

		post_connect(std::move(results));
	});
}

void net::core::tcp_client::post_connect(boost::asio::ip::tcp::resolver::results_type results)
{
	if (current_state.load() != state::connecting) return;

	boost::asio::async_connect(socket, results,
	[this](boost::system::error_code error, const boost::asio::ip::tcp::endpoint&)
	{
		// Socket 에러?
		if (error)
		{
			SPDLOG_WARN("socket error : {}.", error.message());
			boost::system::error_code ignored;
			socket.close(ignored);
			current_state.store(state::disconnected);
			return;
		}

		// 커넥션 등록
		auto new_connection = std::make_shared<connection>(context, std::move(socket), net::common::connection_id_generator::default_id);
		tbb::concurrent_hash_map<net::common::connection_id, std::shared_ptr<connection>>::accessor accessor;
		if (connections.insert(accessor, net::common::connection_id_generator::default_id))
		{
			accessor->second = std::move(new_connection);
		}

		current_state.store(state::connected);
	});
}

void net::core::tcp_client::create_workers()
{
	context_workers.clear();

	// 메인 스레드를 제외한 나머지 스레드를 asio 디스패치 처리에 등록
	const unsigned int hardware_thread_count = std::thread::hardware_concurrency();
	const unsigned int thread_count = hardware_thread_count > 1 ? hardware_thread_count - 1 : 1;
	for (unsigned int i = 0; i < thread_count; i++)
	{
		context_workers.emplace_back([this]()
		{
			context.run();
		});
	}
}

void net::core::tcp_client::delete_workers()
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

void net::core::tcp_client::close()
{
	// 이미 disconnected
	if (current_state.exchange(state::disconnected) == state::disconnected) return;

	// 리졸버 초기화
	resolver.cancel();

	// 내 소켓 닫기
	if (socket.is_open())
	{
		boost::system::error_code error;
		socket.close(error);
	}
	socket.release();

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
