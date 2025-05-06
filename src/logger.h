#pragma once
#include <fstream>
#include <sstream>
#include <ctime>
#include <windows.h>

inline void log_line(const char* msg)
{
    static std::ofstream file("heic_demo.log", std::ios::app);
    // timestamp
    std::time_t t = std::time(nullptr);
    std::tm tm; localtime_s(&tm, &t);
    char ts[32]; std::strftime(ts, sizeof(ts), "[%F %T] ", &tm);

    file << ts << msg << '\n';
    file.flush();                 // keep it live even if we crash
    OutputDebugStringA(ts);
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}
#define LOG_MSG(x) do{ std::ostringstream _s; _s << x; log_line(_s.str().c_str()); }while(0)
