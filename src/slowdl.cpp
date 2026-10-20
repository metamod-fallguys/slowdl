#include <extdll.h> // always

#include <meta_api.h> // of course

#include <filesystem>
#include <string>

#include "slowdl.h"

// cvar_t holds non-const strings, so keep the initial values in writable storage.
static char g_szKickCvarName[]        = "sv_slowdl_kick";
static char g_szKickCvarValue[]       = "0";
static char g_szConsoleLogCvarName[]  = "sv_slowdl_console_log";
static char g_szConsoleLogCvarValue[] = "1";
static char g_szFileLogCvarName[]     = "sv_slowdl_file_log";
static char g_szFileLogCvarValue[]    = "1";

cvar_t g_sv_slowdl_kick        = {g_szKickCvarName, g_szKickCvarValue, FCVAR_SERVER};
cvar_t g_sv_slowdl_console_log = {g_szConsoleLogCvarName, g_szConsoleLogCvarValue, FCVAR_SERVER};
cvar_t g_sv_slowdl_file_log    = {g_szFileLogCvarName, g_szFileLogCvarValue, FCVAR_SERVER};

bool                            g_bIsSvenCoop = false;
CSlowDLMonitor                  g_SlowDLMonitor;
std::unique_ptr<CSlowDLFileLog> g_SlowDLFileLog;

void SlowDL_Init(const char* gameDirectory)
{
    std::string gameName = std::filesystem::path(gameDirectory).filename().string();

    for (char& c : gameName)
    {
        if (c >= 'A' && c <= 'Z')
            c = char(c + ('a' - 'A'));
    }

    g_bIsSvenCoop   = gameName == "svencoop";
    g_SlowDLFileLog = std::make_unique<CSlowDLFileLog>(gameDirectory);
}

void SlowDL_Shutdown()
{
    g_SlowDLFileLog.reset();
}

void SlowDL_RegisterCvars()
{
    g_engfuncs.pfnCVarRegister(&g_sv_slowdl_kick);
    g_engfuncs.pfnCVarRegister(&g_sv_slowdl_console_log);
    g_engfuncs.pfnCVarRegister(&g_sv_slowdl_file_log);
}

slowdl_settings_t SlowDL_GetSettings()
{
    // Metamod registers its own copy of plugin cvars. Read the engine-owned
    // values so both defaults and subsequent administrator changes take effect.
    slowdl_settings_t settings;

    settings.kick        = g_engfuncs.pfnCVarGetFloat(g_sv_slowdl_kick.name) != 0;
    settings.console_log = g_engfuncs.pfnCVarGetFloat(g_sv_slowdl_console_log.name) != 0;
    settings.file_log    = g_engfuncs.pfnCVarGetFloat(g_sv_slowdl_file_log.name) != 0;

    return settings;
}
