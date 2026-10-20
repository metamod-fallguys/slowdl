#include <extdll.h> // always

#include <meta_api.h> // of course

#include "enginedef.h"
#include "slowdl.h"

#define REQUIRED_INTERFACE_MAJOR_VERSION 5
#define REQUIRED_INTERFACE_MINOR_VERSION 17

// Must provide at least one of these..
static META_FUNCTIONS gMetaFunctionTable = {
    NULL,          // pfnGetEntityAPI				HL SDK; called before game DLL
    NULL,          // pfnGetEntityAPI_Post			META; called after game DLL
    GetEntityAPI2, // pfnGetEntityAPI2				HL SDK2; called before game DLL
    NULL,          // pfnGetEntityAPI2_Post		META; called after game DLL
    NULL,          // pfnGetNewDLLFunctions		HL SDK2; called before game DLL
    NULL,          // pfnGetNewDLLFunctions_Post	META; called after game DLL
    NULL,          // pfnGetEngineFunctions	META; called before HL engine
    NULL,          // pfnGetEngineFunctions_Post	META; called after HL engine
    NULL,          // pfnGetStudioBlendingInterface 2022/07/02 Added by hzqst
    NULL,          // pfnGetStudioBlendingInterface_Post 2022/07/02 Added by hzqst
};

// Description of plugin
plugin_info_t Plugin_info = {
    META_INTERFACE_VERSION,                       // ifvers
    "SlowDL",                                     // name
    "1.0",                                        // version
    "2026",                                       // date
    "hzqst",                                      // author
    "https://github.com/metamod-fallguys/slowdl", // url
    "SLOWDL",                                     // logtag, all caps please
    PT_ANYTIME,                                   // (when) loadable
    PT_NEVER,                                     // (when) unloadable
};

// Global vars from metamod:
meta_globals_t*  gpMetaGlobals;   // metamod globals
gamedll_funcs_t* gpGamedllFuncs;  // gameDLL function tables
mutil_funcs_t*   gpMetaUtilFuncs; // metamod utility functions

// Accept "<major>:<minor>" with the required major version and at least the
// required minor version. Trailing characters are rejected.
static bool IsCompatibleInterfaceVersion(const char* interfaceVersion)
{
    if (!interfaceVersion)
        return false;

    int  iMajorVersion = 0;
    int  iMinorVersion = 0;
    char trailing      = 0;

    if (sscanf(interfaceVersion, "%d:%d%c", &iMajorVersion, &iMinorVersion, &trailing) != 2)
        return false;

    return iMajorVersion == REQUIRED_INTERFACE_MAJOR_VERSION && iMinorVersion >= REQUIRED_INTERFACE_MINOR_VERSION;
}

// Resolve symbolName through metamod's gamedata catalog, retrying
// alternativeSymbolName when the catalog has no entry for symbolName.
static bool ResolveEngineSymbol(void* engineBase, const char* symbolName, meta_gamesymbol_kind_t kind, void** pAddress, const char* alternativeSymbolName = NULL)
{
    auto status = gpMetaUtilFuncs->pfnResolveGameSymbol(engineBase, symbolName, kind, pAddress);

    if (status == META_GAMESYMBOL_SYMBOL_NOT_FOUND && alternativeSymbolName)
        status = gpMetaUtilFuncs->pfnResolveGameSymbol(engineBase, alternativeSymbolName, kind, pAddress);

    if (status == META_GAMESYMBOL_OK)
        return true;

    LOG_ERROR(PLID, "Cannot resolve %s: %s", symbolName, gpMetaUtilFuncs->pfnGetGameSymbolStatusString(status));
    return false;
}

#define RESOLVE_ENGINE_FUNCTION(name) ResolveEngineSymbol(engineBase, name##_GameSymbol, META_GAMESYMBOL_KIND_FUNCTION, (void**)&g_pfn_##name, name##_GameSymbolAlternative)
#define RESOLVE_ENGINE_VARIABLE(name) ResolveEngineSymbol(engineBase, name##_GameSymbol, META_GAMESYMBOL_KIND_GLOBAL, (void**)&name)

// Metamod requesting info about this plugin:
//  ifvers			(given) interface_version metamod is using
//  pPlugInfo		(requested) struct with info about plugin
//  pMetaUtilFuncs	(given) table of utility functions provided by metamod
C_DLLEXPORT int Meta_Query(const char* interfaceVersion, plugin_info_t** pPlugInfo, mutil_funcs_t* pMetaUtilFuncs)
{
    if (!IsCompatibleInterfaceVersion(interfaceVersion))
    {
        pMetaUtilFuncs->pfnLogError(PLID, "Metamod interface 5:17 or newer compatible 5.x is required");
        return FALSE;
    }

    // Give metamod our plugin_info struct
    *pPlugInfo = &Plugin_info;
    // Get metamod utility function table.
    gpMetaUtilFuncs = pMetaUtilFuncs;
    return TRUE;
}

C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME    now,
                            META_FUNCTIONS*  pFunctionTable,
                            meta_globals_t*  pMGlobals,
                            gamedll_funcs_t* pGamedllFuncs)
{
    if (!pFunctionTable || !pMGlobals || !gpGlobals)
        return FALSE;

    gpMetaGlobals  = pMGlobals;
    gpGamedllFuncs = pGamedllFuncs;

    auto engineBase = gpMetaUtilFuncs->pfnGetEngineBase();

    if (!engineBase)
        return FALSE;

    if (!RESOLVE_ENGINE_FUNCTION(SV_BeginFileDownload_f))
        return FALSE;

    if (!RESOLVE_ENGINE_FUNCTION(SV_DropClient))
        return FALSE;

    if (!RESOLVE_ENGINE_VARIABLE(host_client))
        return FALSE;

    if (!RESOLVE_ENGINE_VARIABLE(cmd_source))
        return FALSE;

    if (!RESOLVE_ENGINE_VARIABLE(sv_player))
        return FALSE;

    char gameDirectory[1024] = {};
    g_engfuncs.pfnGetGameDir(gameDirectory);

    SlowDL_Init(gameDirectory);

    // Only startup has a later Metamod transaction commit.
    if (!InstallEngineHooks(now == PT_STARTUP))
    {
        SlowDL_Shutdown();
        LOG_ERROR(PLID, "Unable to install the UDP download hook");
        return FALSE;
    }

    SlowDL_RegisterCvars();

    memcpy(pFunctionTable, &gMetaFunctionTable, sizeof(META_FUNCTIONS));
    return TRUE;
}

// Metamod detaching plugin from the server.
// now		(given) current phase, ie during map, etc
// reason	(given) why detaching (refresh, console unload, forced unload, etc)
C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME /* now */,
                            PL_UNLOAD_REASON /* reason */)
{
    if (!UninstallEngineHooks())
        return FALSE;

    SlowDL_Shutdown();
    return TRUE;
}
