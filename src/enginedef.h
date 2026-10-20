#pragma once

#include "gamesymbols.h"

// Engine private types

// client_t is kept opaque so the plugin does not depend on private field offsets.
typedef void client_t;

typedef enum
{
    src_client,  // came in over a net connection as a clc_stringcmd
    src_command, // from the command buffer
} cmd_source_t;

// Engine private functions

// SV_BeginFileDownload_f, the handler of the "dlfile" client command
typedef void (*fnSV_BeginFileDownload_f)();
void NewSV_BeginFileDownload_f();
PRIVATE_FUNCTION_EXTERN(SV_BeginFileDownload_f);

// SV_DropClient
typedef void (*fnSV_DropClient)(client_t* cl, qboolean crash, char* fmt, ...);
PRIVATE_FUNCTION_EXTERN(SV_DropClient);

// Engine private variables

extern client_t**    host_client;
extern cmd_source_t* cmd_source;
extern edict_t**     sv_player;

extern hook_t* g_phook_SV_BeginFileDownload_f;

// Startup loads defer hook activation to the Metamod transaction commit;
// runtime loads must enable the hook immediately.
bool InstallEngineHooks(bool bTransaction);
bool UninstallEngineHooks();
