#include "monitor.h"
#include "file_log.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

#define CHECK(condition)                                   \
    do                                                     \
    {                                                      \
        if (!(condition))                                  \
        {                                                  \
            std::cerr << __LINE__ << ": " #condition "\n"; \
            return 1;                                      \
        }                                                  \
    } while (0)

// Records every decision as one character: F(orward), D(rop), C(onsole), L(og file)
class CSpySlowDLSink : public ISlowDLSink
{
public:
    void ForwardRequest() override
    {
        ++m_iForwards;
        m_Sequence += 'F';
    }

    void DropClient() override
    {
        ++m_iDrops;
        m_Sequence += 'D';
    }

    void PrintConsole(const std::string& line) override
    {
        ++m_iConsoles;
        m_LastLine = line;
        m_Sequence += 'C';
    }

    bool AppendFileLog(const std::string& line) override
    {
        ++m_iFiles;
        m_LastLine = line;
        m_Sequence += 'L';
        return m_bWritable;
    }

    void ReportFileLogError() override
    {
        ++m_iErrors;
    }

    int         m_iForwards = 0;
    int         m_iDrops    = 0;
    int         m_iConsoles = 0;
    int         m_iFiles    = 0;
    int         m_iErrors   = 0;
    bool        m_bWritable = true;
    std::string m_LastLine;
    std::string m_Sequence;
};

int main()
{
    // Classification

    const std::string decal = "!MD5" + std::string(32, 'a');

    CHECK(SlowDL_ClassifyDownload(decal, true, "test") == SlowDLException_CustomDecal);
    CHECK(SlowDL_ClassifyDownload("!md5" + std::string(32, 'a'), false, "test") == SlowDLException_CustomDecal);

    for (const std::string name : std::initializer_list<std::string>{"!anything", "!MD5", decal + "a", "!ModuleC.dll"})
    {
        CHECK(SlowDL_ClassifyDownload(name, true, "test") == SlowDLException_None);
    }

    CHECK(SlowDL_ClassifyDownload("MAPS\\SOUNDCACHE\\TEST.TXT", true, "test") == SlowDLException_SoundCache);

    for (const std::string name : {"soundcache.txt", "maps/soundcache/other.txt", "maps/soundcache/../test.txt", "maps/soundcache/test.txt.bak"})
    {
        CHECK(SlowDL_ClassifyDownload(name, true, "test") == SlowDLException_None);
    }

    CHECK(SlowDL_ClassifyDownload("maps/soundcache/test.txt", false, "test") == SlowDLException_None);
    CHECK(SlowDL_ClassifyDownload("maps/soundcache/.txt", true, "") == SlowDLException_None);

    // Every cvar combination for ordinary and exempt requests

    slowdl_request_t request{true, true, "models/test.mdl", "test", true, "player\n\"", 42, "STEAM_0:1:7"};

    for (int switches = 0; switches < 8; ++switches)
    {
        for (const std::string filename : {std::string("models/test.mdl"), decal, std::string("maps/soundcache/test.txt")})
        {
            request.filename = filename;

            slowdl_settings_t settings{bool(switches & 1), bool(switches & 2), bool(switches & 4)};
            CSlowDLMonitor    monitor;
            CSpySlowDLSink    spy;

            monitor.HandleRequest(request, settings, spy);

            const bool kick = settings.kick && filename == "models/test.mdl";

            CHECK(spy.m_iDrops == int(kick));
            CHECK(spy.m_iForwards == int(!kick));
            CHECK(spy.m_iConsoles == int(settings.console_log));
            CHECK(spy.m_iFiles == int(settings.file_log));
            CHECK(spy.m_Sequence.back() == (kick ? 'D' : 'F'));
            CHECK(spy.m_LastLine.find('\n') == std::string::npos);

            if (settings.console_log || settings.file_log)
            {
                CHECK(spy.m_LastLine.find("42") != std::string::npos);
                CHECK(spy.m_LastLine.find("STEAM_0:1:7") != std::string::npos);
            }
        }
    }

    // Invalid contexts are forwarded without logging

    for (int invalid = 0; invalid < 3; ++invalid)
    {
        auto invalidRequest = request;

        if (invalid == 0)
            invalidRequest.client_source = false;

        if (invalid == 1)
            invalidRequest.has_client = false;

        if (invalid == 2)
            invalidRequest.filename.clear();

        CSlowDLMonitor monitor;
        CSpySlowDLSink spy;

        monitor.HandleRequest(invalidRequest, {true, true, true}, spy);

        CHECK(spy.m_Sequence == "F");
    }

    // File write failures are reported once until writing succeeds again

    CSlowDLMonitor monitor;
    CSpySlowDLSink spy;

    spy.m_bWritable  = false;
    request.filename = "models/test.mdl";

    monitor.HandleRequest(request, {true, false, true}, spy);
    monitor.HandleRequest(request, {true, false, true}, spy);

    CHECK(spy.m_iErrors == 1 && spy.m_iDrops == 2 && spy.m_iForwards == 0);

    spy.m_bWritable = true;
    monitor.HandleRequest(request, {false, false, true}, spy);

    spy.m_bWritable = false;
    monitor.HandleRequest(request, {false, false, true}, spy);

    CHECK(spy.m_iErrors == 2 && spy.m_iForwards == 2);

    // Log escaping

    CHECK(SlowDL_EscapeLogField("a\r\nb\t\"\\") == "a\\r\\nb\\t\\\"\\\\");

    // File log

    const auto root = std::filesystem::temp_directory_path() /
        ("slowdl-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

    CHECK(std::filesystem::create_directory(root));

    CSlowDLFileLog log(root.string());

    CHECK(log.Append("request", 0));

    int count = 0;

    for (const auto& file : std::filesystem::directory_iterator(root / "logs"))
    {
        std::ifstream input(file.path());
        std::string   line;

        CHECK(bool(std::getline(input, line)));
        CHECK(line.find("request") != std::string::npos);

        ++count;
    }

    CHECK(count == 1);

    // A regular file in place of the game directory cannot hold logs/
    std::ofstream(root / "blocked") << "file";

    CSlowDLFileLog blocked((root / "blocked").string());

    CHECK(!blocked.Append("request", 0));

    std::filesystem::remove_all(root);

    std::cout << "SlowDL behavior tests passed\n";
    return 0;
}
