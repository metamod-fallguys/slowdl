#ifndef SLOWDL_MONITOR_H
#define SLOWDL_MONITOR_H

#include <string>

namespace SlowDL
{
enum class Exception
{
    None,
    CustomDecal,
    SoundCache
};
Exception   Classify(const std::string& filename, bool svenCoop, const std::string& map);
std::string Escape(const std::string& text);

struct Settings
{
    bool kick;
    bool consoleLog;
    bool fileLog;
};
struct Request
{
    bool        clientSource = false;
    bool        hasClient    = false;
    std::string filename;
    std::string map;
    bool        svenCoop = false;
    std::string player   = "unknown";
    int         userId   = -1;
    std::string authId   = "unknown";
};
class Sink
{
public:
    virtual ~Sink()                               = default;
    virtual void Forward()                        = 0;
    virtual void Drop()                           = 0;
    virtual void Console(const std::string& line) = 0;
    virtual bool File(const std::string& line)    = 0;
    virtual void FileError()                      = 0;
};
class Monitor
{
public:
    void Handle(const Request& request, const Settings& settings, Sink& sink);

private:
    bool fileFailed = false;
};
} // namespace SlowDL
#endif
