#include "Utils.h"
#include <iostream>
#include <thread>

namespace Utils
{
    void Log(const std::string& msg)
    {
        std::cout << "[*] " << msg << std::endl;
    }

    void PressKeys(const std::string& keys)
    {
        for (char c : keys)
        {
            BYTE vkCode = static_cast<BYTE>(VkKeyScan(c) & 0xFF);
            keybd_event(vkCode, 0, KEYEVENTF_EXTENDEDKEY, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }
}
