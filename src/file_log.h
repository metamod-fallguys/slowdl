#ifndef SLOWDL_FILE_LOG_H
#define SLOWDL_FILE_LOG_H

#include <ctime>
#include <string>

namespace SlowDL
{
class FileLog
{
public:
    explicit FileLog(const std::string& gameDirectory) : directory(gameDirectory) {}
    bool Append(const std::string& line, std::time_t timestamp);

private:
    std::string directory;
};
} // namespace SlowDL
#endif
