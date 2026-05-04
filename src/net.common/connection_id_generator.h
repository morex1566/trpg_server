#pragma once
#include <atomic>
#include <cstdint>

namespace net::common
{
	using connection_id = uint64_t;
}

namespace net::common
{
	/// <summary>
	/// 1 씩 증가하는 CAS기반 uint64_t형 고유값 생성기
	/// </summary>
	class connection_id_generator
	{
	public:
		connection_id_generator() = delete;

		static connection_id generate();

	private:

		static inline std::atomic<connection_id> last_connection_id = 0;
	};
}