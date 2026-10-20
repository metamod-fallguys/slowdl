#include "monitor.h"

// "!MD5" followed by 32 hexadecimal digits
#define CUSTOM_DECAL_PREFIX          "!md5"
#define CUSTOM_DECAL_PREFIX_LENGTH   (sizeof(CUSTOM_DECAL_PREFIX) - 1)
#define CUSTOM_DECAL_FILENAME_LENGTH (CUSTOM_DECAL_PREFIX_LENGTH + 32)

#define SOUNDCACHE_DIRECTORY "maps/soundcache/"
#define SOUNDCACHE_EXTENSION ".txt"

// Unify slash style and ASCII case.
static std::string NormalizePath(std::string text)
{
    for (char& c : text)
    {
        if (c == '\\')
            c = '/';

        if (c >= 'A' && c <= 'Z')
            c = char(c + ('a' - 'A'));
    }

    return text;
}

SlowDLException SlowDL_ClassifyDownload(const std::string& filename, bool svencoop, const std::string& mapname)
{
    const auto normalized = NormalizePath(filename);

    if (normalized.size() == CUSTOM_DECAL_FILENAME_LENGTH && normalized.compare(0, CUSTOM_DECAL_PREFIX_LENGTH, CUSTOM_DECAL_PREFIX) == 0)
        return SlowDLException_CustomDecal;

    // Only the soundcache of the current map is exempt.
    if (svencoop && !mapname.empty() && normalized == SOUNDCACHE_DIRECTORY + NormalizePath(mapname) + SOUNDCACHE_EXTENSION)
        return SlowDLException_SoundCache;

    return SlowDLException_None;
}

std::string SlowDL_EscapeLogField(const std::string& text)
{
    constexpr char hexDigits[] = "0123456789abcdef";

    std::string result;

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
            {
                // Control characters and DEL
                if (c < 0x20 || c == 0x7f)
                {
                    result += "\\x";
                    result += hexDigits[c >> 4];
                    result += hexDigits[c & 0x0f];
                }
                else
                {
                    result += char(c);
                }
                break;
            }
        }
    }

    return result;
}

static const char* GetActionName(bool kick, SlowDLException exception)
{
    if (kick)
        return "kick";

    if (exception == SlowDLException_CustomDecal)
        return "allow-customdecal";

    if (exception == SlowDLException_SoundCache)
        return "allow-soundcache";

    return "allow";
}

void CSlowDLMonitor::HandleRequest(const slowdl_request_t& request, const slowdl_settings_t& settings, ISlowDLSink& sink)
{
    // Console commands and malformed requests are passed through unchanged.
    if (!request.client_source || !request.has_client || request.filename.empty())
    {
        sink.ForwardRequest();
        return;
    }

    const auto exception = SlowDL_ClassifyDownload(request.filename, request.svencoop, request.mapname);
    const bool kick      = settings.kick && exception == SlowDLException_None;

    if (settings.console_log || settings.file_log)
    {
        const std::string line = "[SLOWDL] player=\"" + SlowDL_EscapeLogField(request.player) +
            "\" userid=" + std::to_string(request.userid) +
            " auth=\"" + SlowDL_EscapeLogField(request.authid) +
            "\" file=\"" + SlowDL_EscapeLogField(request.filename) +
            "\" action=" + GetActionName(kick, exception);

        if (settings.console_log)
        {
            sink.PrintConsole(line);
        }

        if (settings.file_log)
        {
            if (sink.AppendFileLog(line))
            {
                m_bFileLogFailed = false;
            }
            else if (!m_bFileLogFailed)
            {
                m_bFileLogFailed = true;
                sink.ReportFileLogError();
            }
        }
    }

    // The request is logged before the client is disconnected.
    if (kick)
    {
        sink.DropClient();
    }
    else
    {
        sink.ForwardRequest();
    }
}
