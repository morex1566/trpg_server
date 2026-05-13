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

		/// <summary>
		/// 인스턴스 생성 방지
		/// </summary>
		connection_id_generator() = delete;

		/// <summary>
		/// 다음 connection id 생성
		/// </summary>
		static uint64_t generate();

	public:

		/// <summary>
		/// 기본 connection id 값
		/// </summary>
		static inline uint64_t default_id = 0;

	private:

		/// <summary>
		/// 마지막으로 발급된 connection id
		/// </summary>
		static inline std::atomic<uint64_t> last_connection_id = 0;
	};
}
