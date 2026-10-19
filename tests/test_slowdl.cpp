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

struct Spy : SlowDL::Sink
{
    int         forwards = 0, drops = 0, consoles = 0, files = 0, errors = 0;
    bool        writable = true;
    std::string line, sequence;
    void        Forward() override
    {
        ++forwards;
        sequence += 'F';
    }
    void Drop() override
    {
        ++drops;
        sequence += 'D';
    }
    void Console(const std::string& text) override
    {
        ++consoles;
        line = text;
        sequence += 'C';
    }
    bool File(const std::string& text) override
    {
        ++files;
        line = text;
        sequence += 'L';
        return writable;
    }
    void FileError() override { ++errors; }
};

int main()
{
    const std::string decal = "!MD5" + std::string(32, 'a');
    CHECK(SlowDL::Classify(decal, true, "test") == SlowDL::Exception::CustomDecal);
    CHECK(SlowDL::Classify("!md5" + std::string(32, 'a'), false, "test") == SlowDL::Exception::CustomDecal);
    for (const std::string name : std::initializer_list<std::string>{"!anything", "!MD5", decal + "a", "!ModuleC.dll"})
        CHECK(SlowDL::Classify(name, true, "test") == SlowDL::Exception::None);
    CHECK(SlowDL::Classify("MAPS\\SOUNDCACHE\\TEST.TXT", true, "test") == SlowDL::Exception::SoundCache);
    for (const std::string name : {"soundcache.txt", "maps/soundcache/other.txt", "maps/soundcache/../test.txt", "maps/soundcache/test.txt.bak"})
        CHECK(SlowDL::Classify(name, true, "test") == SlowDL::Exception::None);
    CHECK(SlowDL::Classify("maps/soundcache/test.txt", false, "test") == SlowDL::Exception::None);
    CHECK(SlowDL::Classify("maps/soundcache/.txt", true, "") == SlowDL::Exception::None);

    SlowDL::Request request{true, true, "models/test.mdl", "test", true, "player\n\"", 42, "STEAM_0:1:7"};
    for (int switches = 0; switches < 8; ++switches)
        for (const std::string filename : {std::string("models/test.mdl"), decal, std::string("maps/soundcache/test.txt")})
        {
            request.filename = filename;
            SlowDL::Settings settings{bool(switches & 1), bool(switches & 2), bool(switches & 4)};
            SlowDL::Monitor  monitor;
            Spy              spy;
            monitor.Handle(request, settings, spy);
            const bool kick = settings.kick && filename == "models/test.mdl";
            CHECK(spy.drops == int(kick));
            CHECK(spy.forwards == int(!kick));
            CHECK(spy.consoles == int(settings.consoleLog));
            CHECK(spy.files == int(settings.fileLog));
            CHECK(spy.sequence.back() == (kick ? 'D' : 'F'));
            CHECK(spy.line.find('\n') == std::string::npos);
            if (settings.consoleLog || settings.fileLog)
            {
                CHECK(spy.line.find("42") != std::string::npos);
                CHECK(spy.line.find("STEAM_0:1:7") != std::string::npos);
            }
        }
    for (int invalid = 0; invalid < 3; ++invalid)
    {
        auto invalidRequest = request;
        if (invalid == 0) invalidRequest.clientSource = false;
        if (invalid == 1) invalidRequest.hasClient = false;
        if (invalid == 2) invalidRequest.filename.clear();
        SlowDL::Monitor monitor;
        Spy             spy;
        monitor.Handle(invalidRequest, {true, true, true}, spy);
        CHECK(spy.sequence == "F");
    }
    SlowDL::Monitor monitor;
    Spy             spy;
    spy.writable     = false;
    request.filename = "models/test.mdl";
    monitor.Handle(request, {true, false, true}, spy);
    monitor.Handle(request, {true, false, true}, spy);
    CHECK(spy.errors == 1 && spy.drops == 2 && spy.forwards == 0);
    spy.writable = true;
    monitor.Handle(request, {false, false, true}, spy);
    spy.writable = false;
    monitor.Handle(request, {false, false, true}, spy);
    CHECK(spy.errors == 2 && spy.forwards == 2);
    CHECK(SlowDL::Escape("a\r\nb\t\"\\") == "a\\r\\nb\\t\\\"\\\\");

    const auto root = std::filesystem::temp_directory_path() /
        ("slowdl-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    CHECK(std::filesystem::create_directory(root));
    SlowDL::FileLog log(root.string());
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
    std::ofstream(root / "blocked") << "file";
    SlowDL::FileLog blocked((root / "blocked").string());
    CHECK(!blocked.Append("request", 0));
    std::filesystem::remove_all(root);
    std::cout << "SlowDL behavior tests passed\n";
    return 0;
}
