#pragma once

#include <string>

// Requests that are exempt from kicking. They are still logged.
enum SlowDLException
{
    SlowDLException_None,
    SlowDLException_CustomDecal,
    SlowDLException_SoundCache,
};

SlowDLException SlowDL_ClassifyDownload(const std::string& filename, bool svencoop, const std::string& mapname);
std::string     SlowDL_EscapeLogField(const std::string& text);

// Snapshot of the sv_slowdl_* cvars for a single request.
typedef struct slowdl_settings_s
{
    bool kick;
    bool console_log;
    bool file_log;
} slowdl_settings_t;

// A dlfile request copied from the engine.
typedef struct slowdl_request_s
{
    bool        client_source = false;
    bool        has_client    = false;
    std::string filename;
    std::string mapname;
    bool        svencoop = false;
    std::string player   = "unknown";
    int         userid   = -1;
    std::string authid   = "unknown";
} slowdl_request_t;

// Receives the decision of CSlowDLMonitor.
class ISlowDLSink
{
public:
    virtual ~ISlowDLSink() = default;

    // Pass the request to the engine download handler.
    virtual void ForwardRequest() = 0;

    // Disconnect the requesting client.
    virtual void DropClient() = 0;

    virtual void PrintConsole(const std::string& line) = 0;

    // Returns false when the line could not be written.
    virtual bool AppendFileLog(const std::string& line) = 0;

    virtual void ReportFileLogError() = 0;
};

class CSlowDLMonitor
{
public:
    CSlowDLMonitor()
    {
        m_bFileLogFailed = false;
    }

    void HandleRequest(const slowdl_request_t& request, const slowdl_settings_t& settings, ISlowDLSink& sink);

private:
    // Only the first of consecutive file log failures is reported.
    bool m_bFileLogFailed;
};
