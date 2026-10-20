#pragma once

#include <signatures_template.h>

// XXXX_GameSymbol is the symbolName of XXXX in the GoldSrc_VibeSignatures
// gamedata (scripts/manifests/slowdl.json and slowdl_windows.json).
// XXXX_GameSymbolAlternative is tried when the catalog has no XXXX_GameSymbol
// entry, since some Linux builds only export the mangled name.

#define SV_BeginFileDownload_f_GameSymbol            "SV_BeginFileDownload_f"
#define SV_BeginFileDownload_f_GameSymbolAlternative "_Z22SV_BeginFileDownload_fv"
#define SV_DropClient_GameSymbol                     "SV_DropClient"
#define SV_DropClient_GameSymbolAlternative          "_Z13SV_DropClientP8client_siPcz"

#define host_client_GameSymbol "host_client"
#define cmd_source_GameSymbol  "cmd_source"
#define sv_player_GameSymbol   "sv_player"
