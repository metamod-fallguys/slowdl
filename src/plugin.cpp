#include "file_log.h"
#include "monitor.h"

#include <filesystem>
#include <memory>

#include <extdll.h>
#include <meta_api.h>
#include <h_export.h>

enginefuncs_t    g_engfuncs;
globalvars_t*    gpGlobals;
meta_globals_t*  gpMetaGlobals;
gamedll_funcs_t* gpGamedllFuncs;
mutil_funcs_t*   gpMetaUtilFuncs;

plugin_info_t Plugin_info = {META_INTERFACE_VERSION, "SlowDL", "1.0", "2026", "hzqst",
                             "https://github.com/metamod-fallguys/slowdl", "SLOWDL", PT_ANYTIME, PT_NEVER};

namespace
{
using DownloadFn = void (*)();
using DropFn     = void (*)(void*, int, char*, ...);
DownloadFn                       originalDownload;
DropFn                           dropClient;
void**                           hostClient;
int*                             cmdSource;
edict_t**                        svPlayer;
hook_t*                          downloadHook;
SlowDL::Monitor                  monitor;
std::unique_ptr<SlowDL::FileLog> fileLog;
bool                             svenCoop;
cvar_t                           kickCvar    = {"sv_slowdl_kick", "0", FCVAR_SERVER};
cvar_t                           consoleCvar = {"sv_slowdl_console_log", "1", FCVAR_SERVER};
cvar_t                           fileCvar    = {"sv_slowdl_file_log", "1", FCVAR_SERVER};

class EngineSink : public SlowDL::Sink
{
public:
    explicit EngineSink(void* client) : client(client) {}
    void Forward() override { originalDownload(); }
    void Drop() override
    {
        static char format[] = "%s";
        dropClient(client, FALSE, format,
                   "UDP downloads are disabled on this server. Please enable FastDL and reconnect.");
    }
    void Console(const std::string& line) override { LOG_CONSOLE(PLID, "%s", line.c_str()); }
    bool File(const std::string& line) override { return fileLog->Append(line, std::time(nullptr)); }
    void FileError() override { LOG_ERROR(PLID, "Unable to append logs/slowdl_YYYYMMDD.log; check the game directory permissions"); }

private:
    void* client;
};

void Download()
{
    SlowDL::Request request;
    request.clientSource = *cmdSource == 0; // src_client, not src_command.
    void* client         = request.clientSource ? *hostClient : nullptr;
    request.hasClient    = client != nullptr;
    if (request.clientSource && request.hasClient && g_engfuncs.pfnCmd_Argc() == 2)
    {
        const char* filename = g_engfuncs.pfnCmd_Argv(1);
        if (filename) request.filename = filename;
        request.map      = STRING(gpGlobals->mapname);
        request.svenCoop = svenCoop;
        edict_t* player  = *svPlayer;
        if (player && !player->free)
        {
            request.player = STRING(player->v.netname);
            if (request.player.empty())
            {
                char*       info = g_engfuncs.pfnGetInfoKeyBuffer(player);
                const char* name = info ? g_engfuncs.pfnInfoKeyValue(info, "name") : nullptr;
                request.player   = name && *name ? name : "unknown";
            }
            request.userId   = g_engfuncs.pfnGetPlayerUserId(player);
            const char* auth = g_engfuncs.pfnGetPlayerAuthId(player);
            if (auth) request.authId = auth;
        }
    }
    EngineSink sink(client);
    // Metamod registers its own copy of plugin cvars. Read the engine-owned
    // values so both defaults and subsequent administrator changes take effect.
    monitor.Handle(request, {g_engfuncs.pfnCVarGetFloat(kickCvar.name) != 0, g_engfuncs.pfnCVarGetFloat(consoleCvar.name) != 0, g_engfuncs.pfnCVarGetFloat(fileCvar.name) != 0}, sink);
}

bool Resolve(void* base, const char* name, meta_gamesymbol_kind_t kind, void** address, const char* alternative = nullptr)
{
    auto status = gpMetaUtilFuncs->pfnResolveGameSymbol(base, name, kind, address);
    if (status == META_GAMESYMBOL_SYMBOL_NOT_FOUND && alternative)
        status = gpMetaUtilFuncs->pfnResolveGameSymbol(base, alternative, kind, address);
    if (status == META_GAMESYMBOL_OK) return true;
    LOG_ERROR(PLID, "Cannot resolve %s: %s", name, gpMetaUtilFuncs->pfnGetGameSymbolStatusString(status));
    return false;
}

int GetAPI(DLL_FUNCTIONS* table, int* version)
{
    if (!table || !version) return FALSE;
    if (*version != INTERFACE_VERSION)
    {
        *version = INTERFACE_VERSION;
        return FALSE;
    }
    memset(table, 0, sizeof(*table));
    return TRUE;
}
} // namespace

C_DLLEXPORT void WINAPI GiveFnptrsToDll(enginefuncs_t* functions, globalvars_t* globals)
{
    memcpy(&g_engfuncs, functions, sizeof(g_engfuncs));
    gpGlobals = globals;
}

C_DLLEXPORT int Meta_Query(const char* version, plugin_info_t** info, mutil_funcs_t* utilities)
{
    int  major = 0, minor = 0;
    char tail = 0;
    if (!version || sscanf(version, "%d:%d%c", &major, &minor, &tail) != 2 || major != 5 || minor < 17)
    {
        utilities->pfnLogError(&Plugin_info, "Metamod interface 5:17 or newer compatible 5.x is required");
        return FALSE;
    }
    *info           = &Plugin_info;
    gpMetaUtilFuncs = utilities;
    return TRUE;
}

C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME now, META_FUNCTIONS* table, meta_globals_t* globals, gamedll_funcs_t* game)
{
    if (!table || !globals || !gpGlobals) return FALSE;
    gpMetaGlobals       = globals;
    gpGamedllFuncs      = game;
    void*      base     = gpMetaUtilFuncs->pfnGetEngineBase();
    DownloadFn download = nullptr;
    if (!base || !Resolve(base, "SV_BeginFileDownload_f", META_GAMESYMBOL_KIND_FUNCTION, (void**)&download, "_Z22SV_BeginFileDownload_fv") ||
        !Resolve(base, "SV_DropClient", META_GAMESYMBOL_KIND_FUNCTION, (void**)&dropClient, "_Z13SV_DropClientP8client_siPcz") ||
        !Resolve(base, "host_client", META_GAMESYMBOL_KIND_GLOBAL, (void**)&hostClient) ||
        !Resolve(base, "cmd_source", META_GAMESYMBOL_KIND_GLOBAL, (void**)&cmdSource) ||
        !Resolve(base, "sv_player", META_GAMESYMBOL_KIND_GLOBAL, (void**)&svPlayer)) return FALSE;
    char directory[1024] = {};
    g_engfuncs.pfnGetGameDir(directory);
    std::string gameName = std::filesystem::path(directory).filename().string();
    for (char& c : gameName)
        if (c >= 'A' && c <= 'Z') c = char(c + ('a' - 'A'));
    svenCoop = gameName == "svencoop";
    fileLog  = std::make_unique<SlowDL::FileLog>(directory);
    // Only startup has a later Metamod transaction commit. Runtime loads must
    // enable the hook immediately.
    downloadHook = gpMetaUtilFuncs->pfnInlineHook((void*)download, (void*)&Download, (void**)&originalDownload, now == PT_STARTUP);
    if (!downloadHook || !originalDownload)
    {
        if (downloadHook) gpMetaUtilFuncs->pfnUnHook(downloadHook);
        downloadHook = nullptr;
        fileLog.reset();
        LOG_ERROR(PLID, "Unable to install the UDP download hook");
        return FALSE;
    }
    g_engfuncs.pfnCVarRegister(&kickCvar);
    g_engfuncs.pfnCVarRegister(&consoleCvar);
    g_engfuncs.pfnCVarRegister(&fileCvar);
    memset(table, 0, sizeof(*table));
    table->pfnGetEntityAPI2 = GetAPI;
    return TRUE;
}

C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME, PL_UNLOAD_REASON)
{
    if (downloadHook && !gpMetaUtilFuncs->pfnUnHook(downloadHook)) return FALSE;
    downloadHook = nullptr;
    fileLog.reset();
    return TRUE;
}
