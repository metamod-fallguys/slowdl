#include "monitor.h"

namespace
{
std::string Normalize(std::string text)
{
    for (char& c : text)
    {
        if (c == '\\') c = '/';
        if (c >= 'A' && c <= 'Z') c = char(c + ('a' - 'A'));
    }
    return text;
}
} // namespace

SlowDL::Exception SlowDL::Classify(const std::string& filename, bool svenCoop, const std::string& map)
{
    const auto normalized = Normalize(filename);
    if (normalized.size() == 36 && normalized.compare(0, 4, "!md5") == 0)
        return Exception::CustomDecal;
    if (svenCoop && !map.empty() && normalized == "maps/soundcache/" + Normalize(map) + ".txt")
        return Exception::SoundCache;
    return Exception::None;
}

std::string SlowDL::Escape(const std::string& text)
{
    constexpr char hex[] = "0123456789abcdef";
    std::string    result;
    for (unsigned char c : text)
    {
        switch (c)
        {
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            default:
                if (c < 0x20 || c == 0x7f)
                {
                    result += "\\x";
                    result += hex[c >> 4];
                    result += hex[c & 15];
                }
                else
                    result += char(c);
        }
    }
    return result;
}

void SlowDL::Monitor::Handle(const Request& request, const Settings& settings, Sink& sink)
{
    if (!request.clientSource || !request.hasClient || request.filename.empty())
    {
        sink.Forward();
        return;
    }
    const auto exception = Classify(request.filename, request.svenCoop, request.map);
    const bool kick      = settings.kick && exception == Exception::None;
    if (settings.consoleLog || settings.fileLog)
    {
        const char*       action = kick ? "kick" : (exception == Exception::CustomDecal ? "allow-customdecal" : exception == Exception::SoundCache ? "allow-soundcache" :
                                                                                                                                                     "allow");
        const std::string line   = "[SLOWDL] player=\"" + Escape(request.player) + "\" userid=" + std::to_string(request.userId) +
            " auth=\"" + Escape(request.authId) + "\" file=\"" + Escape(request.filename) + "\" action=" + action;
        if (settings.consoleLog) sink.Console(line);
        if (settings.fileLog)
        {
            if (sink.File(line)) fileFailed = false;
            else if (!fileFailed)
            {
                fileFailed = true;
                sink.FileError();
            }
        }
    }
    if (kick) sink.Drop();
    else
        sink.Forward();
}
