/**
 * @file DekiGPSPackage.cpp
 * @brief Package entry point for deki-gps
 */
#include "DekiGPSPackage.h"
#include "interop/DekiPlugin.h"
#include "DekiLogSystem.h"
#include "DekiGPS.h"

#ifdef DEKI_EDITOR

extern void DekiGPS_RegisterComponents();
extern int  DekiGPS_GetAutoComponentCount();
extern const DekiComponentMeta* DekiGPS_GetAutoComponentMeta(int index);

static bool s_GPSRegistered = false;

extern "C" {

DEKI_GPS_API int DekiGPS_EnsureRegistered(void)
{
    if (s_GPSRegistered)
        return DekiGPS_GetAutoComponentCount();
    s_GPSRegistered = true;
    DekiGPS_RegisterComponents();
    return DekiGPS_GetAutoComponentCount();
}

DEKI_PLUGIN_API const char* DekiPlugin_GetName(void)    { return "Deki GPS Package"; }
DEKI_PLUGIN_API const char* DekiPlugin_GetVersion(void)
{
#ifdef DEKI_PACKAGE_VERSION
    return DEKI_PACKAGE_VERSION;
#else
    return "0.0.0-dev";
#endif
}
DEKI_PLUGIN_API int  DekiPlugin_Init(void)             { DEKI_LOG_INFO("[deki-gps] DekiPlugin_Init"); return 0; }
DEKI_PLUGIN_API void DekiPlugin_Shutdown(void)
{
    s_GPSRegistered = false;
    // Null the provider so a hot-reload doesn't leave a dangling pointer to a
    // driver instance whose .text is about to be unloaded with the DLL. The
    // driver itself (DesktopGPSComponent's s_Driver) is intentionally leaked,
    // matching the embedded NEO6MGPSComponent pattern.
    DekiGPS::SetCurrent(nullptr);
}
DEKI_PLUGIN_API int  DekiPlugin_GetComponentCount(void){ return DekiGPS_GetAutoComponentCount(); }
DEKI_PLUGIN_API const DekiComponentMeta* DekiPlugin_GetComponentMeta(int index)
{
    return DekiGPS_GetAutoComponentMeta(index);
}
DEKI_PLUGIN_API void DekiPlugin_RegisterComponents(void)
{
    int n = DekiGPS_EnsureRegistered();
    DEKI_LOG_INFO("[deki-gps] DekiPlugin_RegisterComponents -> %d component(s)", n);
}


} // extern "C"

#endif // DEKI_EDITOR
