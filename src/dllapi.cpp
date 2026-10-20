#include <extdll.h>

#include <dllapi.h>
#include <meta_api.h>

// The plugin hooks no DLL functions; Metamod only requires a valid table.
C_DLLEXPORT int GetEntityAPI2(DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion)
{
    if (!pFunctionTable || !interfaceVersion)
        return FALSE;

    if (*interfaceVersion != INTERFACE_VERSION)
    {
        // Tell metamod what version we had, so it can figure out who is out of date.
        *interfaceVersion = INTERFACE_VERSION;
        return FALSE;
    }

    memset(pFunctionTable, 0, sizeof(DLL_FUNCTIONS));
    return TRUE;
}
