#include "DekiGPSModule.h"
#include "interop/DekiPlugin.h"
#include "DekiGPSProvider.h"
#include "nmea/NMEAGPS.h"

namespace
{
struct DekiGPSBackendInit
{
    DekiGPSBackendInit()
    {
        DekiGPSProvider::SetFactory([]() -> IDekiGPS* { return new NMEAGPS(); });
    }
};
static DekiGPSBackendInit s_gps_init;
}

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

DEKI_PLUGIN_API const char* DekiPlugin_GetName(void)    { return "Deki GPS Module"; }
DEKI_PLUGIN_API const char* DekiPlugin_GetVersion(void)
{
#ifdef DEKI_MODULE_VERSION
    return DEKI_MODULE_VERSION;
#else
    return "0.0.0-dev";
#endif
}
DEKI_PLUGIN_API const char* DekiPlugin_GetReflectionJson(void) { return "{}"; }
DEKI_PLUGIN_API int  DekiPlugin_Init(void)             { return 0; }
DEKI_PLUGIN_API void DekiPlugin_Shutdown(void)         { s_GPSRegistered = false; }
DEKI_PLUGIN_API int  DekiPlugin_GetComponentCount(void){ return DekiGPS_GetAutoComponentCount(); }
DEKI_PLUGIN_API const DekiComponentMeta* DekiPlugin_GetComponentMeta(int index)
{
    return DekiGPS_GetAutoComponentMeta(index);
}
DEKI_PLUGIN_API void DekiPlugin_RegisterComponents(void) { DekiGPS_EnsureRegistered(); }

DEKI_PLUGIN_API int DekiPlugin_GetFeatureCount(void) { return 0; }
DEKI_PLUGIN_API const struct DekiModuleFeatureInfo* DekiPlugin_GetFeature(int) { return nullptr; }

} // extern "C"

#endif // DEKI_EDITOR
