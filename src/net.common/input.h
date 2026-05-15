#pragma once

#include <Windows.h>

class input
{
private:
    // 정적 멤버 변수 선언 (m_ 접두사 사용)
    static bool key_current[256];
    static bool key_previous[256];

public:
    // 인스턴스 생성 방지 (선택 사항)
    input() = delete;

    // 매 프레임 최상단에서 호출 (예: input_manager::update())
    static void update()
    {
        memcpy(key_previous, key_current, sizeof(key_current));

        for (int i = 0; i < 256; ++i)
        {
            if (GetAsyncKeyState(i) & 0x8000)
            {
                key_current[i] = true;
            }
            else
            {
                key_current[i] = false;
            }
        }
    }

    static bool get_key(int vk_code)
    {
        return key_current[vk_code];
    }

    static bool get_key_down(int vk_code)
    {
        return key_current[vk_code] && !key_previous[vk_code];
    }

    static bool get_key_up(int vk_code)
    {
        return !key_current[vk_code] && key_previous[vk_code];
    }
};

inline bool input::key_current[256] = { false, };
inline bool input::key_previous[256] = { false, };