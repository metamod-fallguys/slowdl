#pragma once

#include <memory>

#include "monitor.h"
#include "file_log.h"

extern cvar_t g_sv_slowdl_kick;
extern cvar_t g_sv_slowdl_console_log;
extern cvar_t g_sv_slowdl_file_log;

extern bool                            g_bIsSvenCoop;
extern CSlowDLMonitor                  g_SlowDLMonitor;
extern std::unique_ptr<CSlowDLFileLog> g_SlowDLFileLog;

void              SlowDL_Init(const char* gameDirectory);
void              SlowDL_Shutdown();
void              SlowDL_RegisterCvars();
slowdl_settings_t SlowDL_GetSettings();
