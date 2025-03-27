//
// Created by 76515 on 2025/3/27.
//

#include "system_up.h"

#include <thread>

#include "logger.h"

#ifdef __WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

void setConsoleOutputCP() {
#ifdef __WIN32
    SetConsoleOutputCP(CP_UTF8);
    std::ios::sync_with_stdio(false);
    std::wcout.imbue(std::locale("")); // 让 std::wcout 也支持 UTF-8
#endif
}
