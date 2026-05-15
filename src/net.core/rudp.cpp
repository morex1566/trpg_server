#include "pch.h"
#include "rudp.h"

net::core::rudp::rudp()
	:	singleton(),
		socket(context),
		work_guard(boost::asio::make_work_guard(context))
{
}

net::core::rudp::~rudp()
{
}

void net::core::rudp::init(boost::asio::ip::port_type port)
{
	mode = mode::SERVER;
	endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);
	acceptor.emplace(context, endpoint);
}

void net::core::rudp::init(const std::string& host, boost::asio::ip::port_type port)
{
	mode = mode::CLIENT;
	endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(host), port);
}