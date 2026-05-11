#pragma once
#include <atomic>
#include <cstdint>

namespace net::common
{
	/// <summary>
	/// 1 씩 증가하는 CAS기반 uint64_t형 고유값 생성기
	/// </summary>
	class connection_id_generator
	{
	public:
		connection_id_generator() = delete;

		static uint64_t generate();

	public:
		static inline uint64_t default_id = 0;

	private:

		static inline std::atomic<uint64_t> last_connection_id = 0;
	};
}