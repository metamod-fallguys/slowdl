#include "file_log.h"

#include <filesystem>
#include <fstream>

bool CSlowDLFileLog::Append(const std::string& line, std::time_t timestamp)
{
    std::tm localTime{};

#ifdef _WIN32
    if (localtime_s(&localTime, &timestamp) != 0)
        return false;
#else
    if (!localtime_r(&timestamp, &localTime))
        return false;
#endif

    char filename[64];
    char dateTime[64];

    if (!std::strftime(filename, sizeof(filename), "slowdl_%Y%m%d.log", &localTime))
        return false;

    if (!std::strftime(dateTime, sizeof(dateTime), "%Y-%m-%d %H:%M:%S", &localTime))
        return false;

    const auto logDirectory = std::filesystem::path(m_GameDirectory) / "logs";

    std::error_code error;
    std::filesystem::create_directories(logDirectory, error);

    if (error)
        return false;

    std::ofstream file(logDirectory / filename, std::ios::app);

    file << dateTime << ' ' << line << '\n';
    file.flush();

    return file.good();
}
