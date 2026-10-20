#include <extdll.h>

#include <meta_api.h>

#include <ctime>

#include "enginedef.h"
#include "slowdl.h"

PRIVATE_FUNCTION_DEFINE(SV_BeginFileDownload_f);
PRIVATE_FUNCTION_DEFINE(SV_DropClient);

client_t**    host_client = NULL;
cmd_source_t* cmd_source  = NULL;
edict_t**     sv_player   = NULL;

// Applies the decision of CSlowDLMonitor to the engine.
class CEngineSlowDLSink : public ISlowDLSink
{
public:
    explicit CEngineSlowDLSink(client_t* client) :
        m_pClient(client)
    {
    }

    void ForwardRequest() override
    {
        g_call_original_SV_BeginFileDownload_f();
    }

    void DropClient() override
    {
        static char format[] = "%s";

        g_pfn_SV_DropClient(m_pClient, FALSE, format, "UDP downloads are disabled on this server. Please enable FastDL and reconnect.");
    }

    void PrintConsole(const std::string& line) override
    {
        LOG_CONSOLE(PLID, "%s", line.c_str());
    }

    bool AppendFileLog(const std::string& line) override
    {
        return g_SlowDLFileLog->Append(line, std::time(nullptr));
    }

    void ReportFileLogError() override
    {
        LOG_ERROR(PLID, "Unable to append logs/slowdl_YYYYMMDD.log; check the game directory permissions");
    }

private:
    client_t* m_pClient;
};

static void CopyPlayerIdentity(edict_t* pPlayer, slowdl_request_t& request)
{
    request.player = STRING(pPlayer->v.netname);

    // netname is not set before the player spawns.
    if (request.player.empty())
    {
        char*       infoBuffer = g_engfuncs.pfnGetInfoKeyBuffer(pPlayer);
        const char* name       = infoBuffer ? g_engfuncs.pfnInfoKeyValue(infoBuffer, "name") : NULL;

        request.player = (name && *name) ? name : "unknown";
    }

    request.userid = g_engfuncs.pfnGetPlayerUserId(pPlayer);

    const char* authid = g_engfuncs.pfnGetPlayerAuthId(pPlayer);

    if (authid)
        request.authid = authid;
}

void NewSV_BeginFileDownload_f()
{
    slowdl_request_t request;

    request.client_source = (*cmd_source == src_client);

    client_t* client = request.client_source ? *host_client : NULL;

    request.has_client = (client != NULL);

    // dlfile <filename>
    if (request.client_source && request.has_client && g_engfuncs.pfnCmd_Argc() == 2)
    {
        const char* filename = g_engfuncs.pfnCmd_Argv(1);

        if (filename)
            request.filename = filename;

        request.mapname  = STRING(gpGlobals->mapname);
        request.svencoop = g_bIsSvenCoop;

        edict_t* pPlayer = *sv_player;

        if (pPlayer && !pPlayer->free)
        {
            CopyPlayerIdentity(pPlayer, request);
        }
    }

    CEngineSlowDLSink sink(client);

    g_SlowDLMonitor.HandleRequest(request, SlowDL_GetSettings(), sink);
}

hook_t* g_phook_SV_BeginFileDownload_f = NULL;

bool InstallEngineHooks(bool bTransaction)
{
    g_phook_SV_BeginFileDownload_f = gpMetaUtilFuncs->pfnInlineHook((void*)g_pfn_SV_BeginFileDownload_f, (void*)NewSV_BeginFileDownload_f, (void**)&g_call_original_SV_BeginFileDownload_f, bTransaction);

    if (!g_phook_SV_BeginFileDownload_f || !g_call_original_SV_BeginFileDownload_f)
    {
        if (g_phook_SV_BeginFileDownload_f)
            gpMetaUtilFuncs->pfnUnHook(g_phook_SV_BeginFileDownload_f);

        g_phook_SV_BeginFileDownload_f = NULL;
        return false;
    }

    return true;
}

bool UninstallEngineHooks()
{
    if (g_phook_SV_BeginFileDownload_f && !gpMetaUtilFuncs->pfnUnHook(g_phook_SV_BeginFileDownload_f))
        return false;

    g_phook_SV_BeginFileDownload_f = NULL;
    return true;
}
