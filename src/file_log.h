#pragma once

#include <ctime>
#include <string>

// Appends records to <gamedir>/logs/slowdl_YYYYMMDD.log
class CSlowDLFileLog
{
public:
    explicit CSlowDLFileLog(const std::string& gameDirectory) :
        m_GameDirectory(gameDirectory)
    {
    }

    // Returns false when the log directory or file cannot be written.
    bool Append(const std::string& line, std::time_t timestamp);

private:
    std::string m_GameDirectory;
};
