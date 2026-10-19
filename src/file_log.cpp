#include "file_log.h"

#include <filesystem>
#include <fstream>

bool SlowDL::FileLog::Append(const std::string& line, std::time_t timestamp)
{
    std::tm local{};
#ifdef _WIN32
    if (localtime_s(&local, &timestamp) != 0) return false;
#else
    if (!localtime_r(&timestamp, &local)) return false;
#endif
    char filename[64], date[64];
    if (!std::strftime(filename, sizeof(filename), "slowdl_%Y%m%d.log", &local) ||
        !std::strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &local)) return false;
    const auto      logs = std::filesystem::path(directory) / "logs";
    std::error_code error;
    std::filesystem::create_directories(logs, error);
    if (error) return false;
    std::ofstream file(logs / filename, std::ios::app);
    file << date << ' ' << line << '\n';
    file.flush();
    return file.good();
}
